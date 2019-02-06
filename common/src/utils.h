#pragma once

#include <string>
#include <arpa/inet.h>
#include <log4cplus/logger.h>

namespace common {

    std::string address_from_sockaddr(const sockaddr *address);

    sockaddr_in make_sockaddr(in_addr_t host, std::uint16_t port);

    sockaddr_in make_sockaddr(const std::string &host, std::uint16_t port);

    log4cplus::Logger make_logger(const std::string &logger_name);

}
