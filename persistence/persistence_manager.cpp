#include "redisclone/persistence/persistence_manager.hpp"
#include "redisclone/common.hpp"
#include "redisclone/parser/command_parser.hpp"
#include <filesystem>
#include <fstream>

namespace redisclone {

PersistenceManager::PersistenceManager(std::string data_dir, std::string snapshot_name, std::string aof_name)
    : data_dir_(std::move(data_dir)) {
    std::filesystem::create_directories(data_dir_);
    snapshot_path_ = data_dir_ + "/" + snapshot_name;
    aof_path_ = data_dir_ + "/" + aof_name;
}

bool PersistenceManager::load(KeyValueStore& store) {
    if (std::filesystem::exists(snapshot_path_)) {
        store.load_snapshot(snapshot_path_);
    }

    if (!std::filesystem::exists(aof_path_)) {
        return true;
    }

    std::ifstream in(aof_path_);
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        auto parsed = CommandParser::parse(line);
        if (!parsed) continue;

        const auto& cmd = *parsed;
        if (cmd.name == "SET" && cmd.args.size() >= 2) {
            std::optional<int> ttl;
            if (cmd.args.size() >= 4 && to_upper_copy(cmd.args[2]) == "EX") {
                ttl = std::stoi(cmd.args[3]);
            }
            store.set(cmd.args[0], cmd.args[1], ttl);
        } else if (cmd.name == "DEL" && !cmd.args.empty()) {
            store.del(cmd.args[0]);
        } else if (cmd.name == "EXPIRE" && cmd.args.size() >= 2) {
            store.expire(cmd.args[0], std::stoi(cmd.args[1]));
        }
    }
    return true;
}

bool PersistenceManager::append_command(const std::string& line) {
    std::lock_guard lock(mutex_);
    std::ofstream out(aof_path_, std::ios::app);
    if (!out) return false;
    out << line << '\n';
    return true;
}

bool PersistenceManager::save_snapshot(KeyValueStore& store) const {
    std::lock_guard lock(mutex_);
    return store.save_snapshot(snapshot_path_);
}

} // namespace redisclone
