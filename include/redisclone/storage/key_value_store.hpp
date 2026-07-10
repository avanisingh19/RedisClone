#pragma once
#include "redisclone/common.hpp"
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace redisclone {

struct Entry {
    std::string value;
    std::optional<TimePoint> expiry;
};

class KeyValueStore {
public:
    bool set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds = std::nullopt);
    bool get(const std::string& key, std::string& value_out);
    bool del(const std::string& key);
    long long expire(const std::string& key, int ttl_seconds);
    long long ttl(const std::string& key);
    std::vector<std::pair<std::string, Entry>> snapshot();
    std::vector<std::string> keys();

    std::size_t size();
    void prune_expired();

    bool save_snapshot(const std::string& path);
    bool load_snapshot(const std::string& path);

private:
    void prune_expired_locked();
    bool is_expired(const Entry& entry) const;
    std::optional<int> remaining_ttl_seconds(const Entry& entry) const;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> data_;
};

} // namespace redisclone
