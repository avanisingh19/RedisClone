#pragma once
#include <optional>
#include <string>
#include <vector>

namespace redisclone {

struct Command {
    std::string name;
    std::vector<std::string> args;
    std::string raw;
};

class CommandParser {
public:
    static std::optional<Command> parse(const std::string& line);
    static std::string canonicalize(const Command& cmd);

private:
    static std::vector<std::string> tokenize(const std::string& line);
};

} // namespace redisclone
