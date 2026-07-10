#pragma once
#include "redisclone/parser/command_parser.hpp"
#include "redisclone/persistence/persistence_manager.hpp"
#include "redisclone/storage/key_value_store.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace redisclone {

class RedisServer {
public:
    RedisServer(int port, std::string data_dir, bool replica_mode = false,
                std::string master_host = "", int master_port = 0);

    int run();

private:
    int port_;
    std::string data_dir_;
    bool replica_mode_;
    std::string master_host_;
    int master_port_;

    std::atomic<bool> running_{true};

    KeyValueStore store_;
    PersistenceManager persistence_;

    std::mutex replicas_mutex_;
    std::vector<int> replica_fds_;

    std::thread cleanup_thread_;
    std::thread replication_thread_;

    void cleanup_loop();
    void replication_loop();

    void handle_connection(int fd);
    void handle_client(int fd);
    void handle_replica(int fd);

    std::string execute_command(const Command& cmd, bool from_replica = false);
    void broadcast_to_replicas(const std::string& line);
    void register_replica(int fd);
    void unregister_replica(int fd);
    void close_all_replicas();
};

} // namespace redisclone
