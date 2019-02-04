#pragma once

#include <string>
#include <arpa/inet.h>

namespace common {

    std::string address_from_sockaddr(sockaddr *address);

    sockaddr_in make_sockaddr(std::uint32_t host, std::uint16_t port);

    sockaddr_in make_sockaddr(const std::string &host, std::uint16_t port);

}