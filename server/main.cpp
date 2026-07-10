#include "redisclone/server/redis_server.hpp"
#include <iostream>
#include <string>

using namespace redisclone;

static int get_int_arg(char** argv, int idx) {
    return std::stoi(argv[idx]);
}

int main(int argc, char** argv) {
    int port = 6379;
    std::string data_dir = "./data";
    bool replica = false;
    std::string master_host;
    int master_port = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = get_int_arg(argv, ++i);
        } else if (arg == "--data-dir" && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (arg == "--replica-of" && i + 2 < argc) {
            replica = true;
            master_host = argv[++i];
            master_port = get_int_arg(argv, ++i);
        } else {
            std::cerr << "Usage: redis-server [--port N] [--data-dir DIR] [--replica-of HOST PORT]\n";
            return 1;
        }
    }

    RedisServer server(port, data_dir, replica, master_host, master_port);
    return server.run();
}
