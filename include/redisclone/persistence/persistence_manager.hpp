#pragma once
#include "redisclone/storage/key_value_store.hpp"
#include <mutex>
#include <string>

namespace redisclone {

class PersistenceManager {
public:
    PersistenceManager(std::string data_dir, std::string snapshot_name = "dump.rdb", std::string aof_name = "appendonly.aof");

    bool load(KeyValueStore& store);
    bool append_command(const std::string& line);
    bool save_snapshot(KeyValueStore& store) const;

    const std::string& snapshot_path() const { return snapshot_path_; }
    const std::string& aof_path() const { return aof_path_; }

private:
    std::string data_dir_;
    std::string snapshot_path_;
    std::string aof_path_;
    mutable std::mutex mutex_;
};

} // namespace redisclone
