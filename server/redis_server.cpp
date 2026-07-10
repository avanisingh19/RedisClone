#include "redisclone/server/redis_server.hpp"
#include "redisclone/common.hpp"
#include "redisclone/networking/socket_utils.hpp"
#include "redisclone/parser/command_parser.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace redisclone {

RedisServer::RedisServer(int port, std::string data_dir, bool replica_mode, std::string master_host, int master_port)
    : port_(port),
      data_dir_(std::move(data_dir)),
      replica_mode_(replica_mode),
      master_host_(std::move(master_host)),
      master_port_(master_port),
      persistence_(data_dir_) {
    persistence_.load(store_);
}

void RedisServer::cleanup_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        store_.prune_expired();
    }
}

void RedisServer::register_replica(int fd) {
    std::lock_guard lock(replicas_mutex_);
    replica_fds_.push_back(fd);
}

void RedisServer::unregister_replica(int fd) {
    std::lock_guard lock(replicas_mutex_);
    replica_fds_.erase(std::remove(replica_fds_.begin(), replica_fds_.end(), fd), replica_fds_.end());
}

void RedisServer::close_all_replicas() {
    std::lock_guard lock(replicas_mutex_);
    for (int fd : replica_fds_) {
        ::close(fd);
    }
    replica_fds_.clear();
}

void RedisServer::broadcast_to_replicas(const std::string& line) {
    std::lock_guard lock(replicas_mutex_);
    std::vector<int> alive;
    alive.reserve(replica_fds_.size());
    for (int fd : replica_fds_) {
        if (send_all(fd, line + "\n")) {
            alive.push_back(fd);
        } else {
            ::close(fd);
        }
    }
    replica_fds_.swap(alive);
}

std::string RedisServer::execute_command(const Command& cmd, bool from_replica) {
    const std::string command = to_upper_copy(cmd.name);

    if (command == "PING") {
        return "+PONG";
    }

    if (command == "INFO") {
        std::ostringstream oss;
        oss << "+redisclone keys=" << store_.size()
            << " mode=" << (replica_mode_ ? "replica" : "master");
        return oss.str();
    }

    if (command == "KEYS") {
        auto keys = store_.keys();
        std::ostringstream oss;
        oss << "*" << keys.size();
        for (const auto& key : keys) {
            oss << "\n$" << key;
        }
        return oss.str();
    }

    if (command == "GET") {
        if (cmd.args.size() != 1) return "-ERR GET requires 1 argument";
        std::string value;
        if (!store_.get(cmd.args[0], value)) return "$-1";
        return "$" + value;
    }

    if (command == "SET") {
        if (replica_mode_ && !from_replica) return "-ERR replica is read-only";
        if (cmd.args.size() < 2) return "-ERR SET requires at least 2 arguments";

        std::optional<int> ttl;
        std::string key = cmd.args[0];
        std::string value = cmd.args[1];
        if (cmd.args.size() >= 4 && to_upper_copy(cmd.args[2]) == "EX") {
            ttl = std::stoi(cmd.args[3]);
        }

        store_.set(key, value, ttl);
        persistence_.append_command(CommandParser::canonicalize(cmd));

        if (!from_replica && !replica_mode_) {
            broadcast_to_replicas(CommandParser::canonicalize(cmd));
        }
        return "+OK";
    }

    if (command == "DEL") {
        if (replica_mode_ && !from_replica) return "-ERR replica is read-only";
        if (cmd.args.size() != 1) return "-ERR DEL requires 1 argument";
        bool deleted = store_.del(cmd.args[0]);
        if (deleted) {
            persistence_.append_command(CommandParser::canonicalize(cmd));
            if (!from_replica && !replica_mode_) {
                broadcast_to_replicas(CommandParser::canonicalize(cmd));
            }
        }
        return ":" + std::to_string(deleted ? 1 : 0);
    }

    if (command == "EXPIRE") {
        if (replica_mode_ && !from_replica) return "-ERR replica is read-only";
        if (cmd.args.size() != 2) return "-ERR EXPIRE requires 2 arguments";
        int ttl = std::stoi(cmd.args[1]);
        long long result = store_.expire(cmd.args[0], ttl);
        if (result) {
            persistence_.append_command(CommandParser::canonicalize(cmd));
            if (!from_replica && !replica_mode_) {
                broadcast_to_replicas(CommandParser::canonicalize(cmd));
            }
        }
        return ":" + std::to_string(result);
    }

    if (command == "TTL") {
        if (cmd.args.size() != 1) return "-ERR TTL requires 1 argument";
        return ":" + std::to_string(store_.ttl(cmd.args[0]));
    }

    if (command == "SAVE") {
        return persistence_.save_snapshot(store_) ? "+OK" : "-ERR failed to save snapshot";
    }

    if (command == "HELP") {
        return "+Commands: PING, SET, GET, DEL, EXPIRE, TTL, KEYS, INFO, SAVE, QUIT";
    }

    if (command == "QUIT") {
        return "+BYE";
    }

    return "-ERR unknown command";
}

void RedisServer::handle_client(int fd) {
    std::string line;
    while (read_line(fd, line)) {
        auto parsed = CommandParser::parse(line);
        if (!parsed) {
            if (!send_all(fd, "-ERR empty command\n")) break;
            continue;
        }

        auto response = execute_command(*parsed, false);
        if (!send_all(fd, response + "\n")) break;
        if (to_upper_copy(parsed->name) == "QUIT") {
            break;
        }
    }
    ::close(fd);
}

void RedisServer::handle_replica(int fd) {
    send_all(fd, "+FULLRESYNC\n");
    auto items = store_.snapshot();
    for (const auto& [key, entry] : items) {
        std::ostringstream oss;
        oss << "SET " << key << " " << entry.value;
        if (entry.expiry.has_value()) {
            auto ttl = std::chrono::duration_cast<std::chrono::seconds>(
                *entry.expiry - Clock::now()).count();
            if (ttl > 0) {
                oss << " EX " << ttl;
            }
        }
        send_all(fd, oss.str() + "\n");
    }
    send_all(fd, "SYNC-END\n");

    register_replica(fd);

    std::string line;
    while (read_line(fd, line)) {
        (void)line;
    }
    unregister_replica(fd);
    ::close(fd);
}

void RedisServer::handle_connection(int fd) {
    std::string first_line;
    if (!read_line(fd, first_line)) {
        ::close(fd);
        return;
    }

    auto trimmed = trim_copy(first_line);
    if (to_upper_copy(trimmed) == "REPLICA") {
        handle_replica(fd);
        return;
    }

    auto parsed = CommandParser::parse(first_line);
    if (!parsed) {
        send_all(fd, "-ERR empty command\n");
        ::close(fd);
        return;
    }

    auto response = execute_command(*parsed, false);
    send_all(fd, response + "\n");
    if (to_upper_copy(parsed->name) == "QUIT") {
        ::close(fd);
        return;
    }

    std::thread([this, fd]() mutable {
        handle_client(fd);
    }).detach();
}

void RedisServer::replication_loop() {
    while (running_) {
        int fd = connect_to_server(master_host_, static_cast<std::uint16_t>(master_port_));
        if (fd < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        send_all(fd, "REPLICA\n");
        std::string line;
        while (read_line(fd, line)) {
            if (line == "+FULLRESYNC" || line == "SYNC-END") {
                continue;
            }

            auto parsed = CommandParser::parse(line);
            if (!parsed) continue;
            execute_command(*parsed, true);
        }
        ::close(fd);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int RedisServer::run() {
    cleanup_thread_ = std::thread([this]() { cleanup_loop(); });

    if (replica_mode_) {
        replication_thread_ = std::thread([this]() { replication_loop(); });
    }

    int server_fd = create_server_socket(static_cast<std::uint16_t>(port_));
    if (server_fd < 0) {
        std::cerr << "Failed to bind on port " << port_ << "\n";
        running_ = false;
        if (cleanup_thread_.joinable()) cleanup_thread_.join();
        if (replication_thread_.joinable()) replication_thread_.join();
        return 1;
    }

    std::cout << "RedisClone listening on port " << port_
              << (replica_mode_ ? " [REPLICA]" : " [MASTER]") << "\n";

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int fd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (fd < 0) {
            continue;
        }

        std::thread([this, fd]() {
            handle_connection(fd);
        }).detach();
    }

    ::close(server_fd);
    close_all_replicas();

    if (cleanup_thread_.joinable()) cleanup_thread_.join();
    if (replication_thread_.joinable()) replication_thread_.join();
    return 0;
}

} // namespace redisclone
