#pragma once
#include <chrono>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

namespace redisclone {

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

inline std::string trim_copy(std::string s) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };

    while (!s.empty() && !not_space(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

inline std::string to_upper_copy(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

inline long long now_epoch_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        Clock::now().time_since_epoch()).count();
}

} // namespace redisclone
