#include "redisclone/parser/command_parser.hpp"
#include "redisclone/common.hpp"
#include <sstream>

namespace redisclone {

std::vector<std::string> CommandParser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    bool escape = false;

    for (char ch : line) {
        if (escape) {
            current.push_back(ch);
            escape = false;
            continue;
        }

        if (ch == '\\') {
            escape = true;
            continue;
        }

        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        if (!in_quotes && std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (escape) {
        current.push_back('\\');
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::optional<Command> CommandParser::parse(const std::string& line) {
    std::string trimmed = trim_copy(line);
    if (trimmed.empty()) return std::nullopt;
    if (trimmed.front() == '#') return std::nullopt;

    auto tokens = tokenize(trimmed);
    if (tokens.empty()) return std::nullopt;

    Command cmd;
    cmd.raw = trimmed;
    cmd.name = to_upper_copy(tokens[0]);
    cmd.args.assign(tokens.begin() + 1, tokens.end());
    return cmd;
}

std::string CommandParser::canonicalize(const Command& cmd) {
    std::ostringstream oss;
    oss << to_upper_copy(cmd.name);
    for (const auto& arg : cmd.args) {
        oss << ' ' << arg;
    }
    return oss.str();
}

} // namespace redisclone
