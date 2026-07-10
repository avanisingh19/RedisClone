#pragma once
#include <cstdint>
#include <string>

namespace redisclone {

int create_server_socket(std::uint16_t port, int backlog = 64);
int connect_to_server(const std::string& host, std::uint16_t port);
bool send_all(int fd, const std::string& data);
bool read_line(int fd, std::string& out);
std::string peer_address(int fd);

} // namespace redisclone
