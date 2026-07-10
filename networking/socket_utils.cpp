#include "redisclone/networking/socket_utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace redisclone {

bool send_all(int fd, const std::string& data) {
    const char* ptr = data.data();
    std::size_t left = data.size();
    while (left > 0) {
        ssize_t n = ::send(fd, ptr, left, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        ptr += n;
        left -= static_cast<std::size_t>(n);
    }
    return true;
}

bool read_line(int fd, std::string& out) {
    out.clear();
    char ch = 0;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n == 0) {
            return false;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (ch == '\r') continue;
        if (ch == '\n') return true;
        out.push_back(ch);
        if (out.size() > 1024 * 1024) return false;
    }
}

int create_server_socket(std::uint16_t port, int backlog) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }

    if (::listen(fd, backlog) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

int connect_to_server(const std::string& host, std::uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        return -1;
    }

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

std::string peer_address(int fd) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (::getpeername(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        return "unknown";
    }
    char buf[INET_ADDRSTRLEN]{};
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
    return std::string(buf) + ":" + std::to_string(ntohs(addr.sin_port));
}

} // namespace redisclone
