#include "redisclone/networking/socket_utils.hpp"
#include "redisclone/common.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace redisclone;

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 6379;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else {
            std::cerr << "Usage: redis-cli [--host HOST] [--port PORT]\n";
            return 1;
        }
    }

    int fd = connect_to_server(host, static_cast<std::uint16_t>(port));
    if (fd < 0) {
        std::cerr << "Could not connect to " << host << ":" << port << "\n";
        return 1;
    }

    std::cout << "Connected. Type commands, QUIT to exit.\n";
    std::string line;
    while (true) {
        std::cout << "redis> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (trim_copy(line).empty()) continue;

        if (!send_all(fd, line + "\n")) break;

        std::string response;
        if (!read_line(fd, response)) break;

        std::cout << response << "\n";
        if (to_upper_copy(trim_copy(line)) == "QUIT") {
            break;
        }
    }

    ::close(fd);
    return 0;
}
