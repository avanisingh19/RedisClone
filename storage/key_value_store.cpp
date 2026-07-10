#include "redisclone/storage/key_value_store.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace redisclone {

bool KeyValueStore::is_expired(const Entry& entry) const {
    if (!entry.expiry.has_value()) return false;
    return Clock::now() >= *entry.expiry;
}

std::optional<int> KeyValueStore::remaining_ttl_seconds(const Entry& entry) const {
    if (!entry.expiry.has_value()) return std::nullopt;
    auto now = Clock::now();
    if (now >= *entry.expiry) return 0;
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(*entry.expiry - now).count();
    if (diff < 0) diff = 0;
    return static_cast<int>(diff);
}

void KeyValueStore::prune_expired_locked() {
    for (auto it = data_.begin(); it != data_.end();) {
        if (is_expired(it->second)) {
            it = data_.erase(it);
        } else {
            ++it;
        }
    }
}

bool KeyValueStore::set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds) {
    std::unique_lock lock(mutex_);
    Entry entry;
    entry.value = value;
    if (ttl_seconds.has_value()) {
        if (*ttl_seconds <= 0) {
            data_.erase(key);
            return false;
        }
        entry.expiry = Clock::now() + std::chrono::seconds(*ttl_seconds);
    }
    data_[key] = std::move(entry);
    return true;
}

bool KeyValueStore::get(const std::string& key, std::string& value_out) {
    std::unique_lock lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) return false;
    if (is_expired(it->second)) {
        data_.erase(it);
        return false;
    }
    value_out = it->second.value;
    return true;
}

bool KeyValueStore::del(const std::string& key) {
    std::unique_lock lock(mutex_);
    return data_.erase(key) > 0;
}

long long KeyValueStore::expire(const std::string& key, int ttl_seconds) {
    std::unique_lock lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) return 0;
    if (is_expired(it->second)) {
        data_.erase(it);
        return 0;
    }
    if (ttl_seconds <= 0) {
        data_.erase(it);
        return 1;
    }
    it->second.expiry = Clock::now() + std::chrono::seconds(ttl_seconds);
    return 1;
}

long long KeyValueStore::ttl(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) return -2;
    if (is_expired(it->second)) {
        data_.erase(it);
        return -2;
    }
    auto ttl_value = remaining_ttl_seconds(it->second);
    if (!ttl_value.has_value()) return -1;
    return *ttl_value;
}

std::vector<std::pair<std::string, Entry>> KeyValueStore::snapshot() {
    std::unique_lock lock(mutex_);
    prune_expired_locked();
    std::vector<std::pair<std::string, Entry>> out;
    out.reserve(data_.size());
    for (const auto& [key, entry] : data_) {
        out.emplace_back(key, entry);
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    return out;
}

std::vector<std::string> KeyValueStore::keys() {
    std::unique_lock lock(mutex_);
    prune_expired_locked();
    std::vector<std::string> out;
    out.reserve(data_.size());
    for (const auto& [key, _] : data_) {
        out.push_back(key);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::size_t KeyValueStore::size() {
    std::unique_lock lock(mutex_);
    prune_expired_locked();
    return data_.size();
}

void KeyValueStore::prune_expired() {
    std::unique_lock lock(mutex_);
    prune_expired_locked();
}

bool KeyValueStore::save_snapshot(const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;

    auto items = snapshot();
    for (const auto& [key, entry] : items) {
        out << key << '\t' << entry.value << '\t';
        if (entry.expiry.has_value()) {
            auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
                entry.expiry->time_since_epoch()).count();
            out << epoch;
        } else {
            out << -1;
        }
        out << '\n';
    }
    return true;
}

bool KeyValueStore::load_snapshot(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    std::unique_lock lock(mutex_);
    data_.clear();

    std::string line;
    while (std::getline(in, line)) {
        if (trim_copy(line).empty()) continue;
        auto first = line.find('\t');
        auto second = line.find('\t', first == std::string::npos ? std::string::npos : first + 1);
        if (first == std::string::npos || second == std::string::npos) continue;

        std::string key = line.substr(0, first);
        std::string value = line.substr(first + 1, second - first - 1);
        std::string expiry_str = line.substr(second + 1);

        Entry entry;
        entry.value = value;
        long long expiry_epoch = std::stoll(expiry_str);
        if (expiry_epoch >= 0) {
            entry.expiry = TimePoint{std::chrono::seconds(expiry_epoch)};
            if (Clock::now() >= *entry.expiry) {
                continue;
            }
        }
        data_[key] = std::move(entry);
    }
    return true;
}

} // namespace redisclone
