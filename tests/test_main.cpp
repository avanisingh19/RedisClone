#include "redisclone/parser/command_parser.hpp"
#include "redisclone/storage/key_value_store.hpp"
#include <cassert>
#include <iostream>

using namespace redisclone;

int main() {
    {
        auto cmd = CommandParser::parse(R"(SET key "hello world")");
        assert(cmd);
        assert(cmd->name == "SET");
        assert(cmd->args.size() == 2);
        assert(cmd->args[1] == "hello world");
    }

    {
        KeyValueStore store;
        assert(store.set("a", "1"));
        std::string value;
        assert(store.get("a", value));
        assert(value == "1");
        assert(store.ttl("a") == -1);
        assert(store.expire("a", 1) == 1);
        assert(store.ttl("a") >= 0);
        assert(store.del("a"));
        assert(store.ttl("a") == -2);
    }

    std::cout << "All tests passed\n";
    return 0;
}
