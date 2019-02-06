#include "utils.h"

#include <cstring>
#include <log4cplus/consoleappender.h>

namespace {

    sockaddr_in make_sockaddr_with_port(std::uint16_t port)
    {
        sockaddr_in sin;
        memset(&sin, 0, sizeof (sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        return sin;
    }

}

namespace common {

    std::string address_from_sockaddr(const sockaddr *address)
    {
        const auto *addr_in{reinterpret_cast<const sockaddr_in *>(address)};
        return inet_ntoa(addr_in->sin_addr);
    }

    sockaddr_in make_sockaddr(in_addr_t host, std::uint16_t port)
    {
        sockaddr_in sin{make_sockaddr_with_port(port)};
        sin.sin_addr.s_addr = htonl(host);
        return sin;
    }

    sockaddr_in make_sockaddr(const std::string &host, std::uint16_t port)
    {
        sockaddr_in sin{make_sockaddr_with_port(port)};
        inet_pton(AF_INET, host.c_str(), &sin.sin_addr.s_addr);
        return sin;
    }

    log4cplus::Logger make_logger(const std::string &logger_name)
    {
        log4cplus::Logger logger{log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(logger_name))};
        log4cplus::SharedAppenderPtr consoleAppender(new log4cplus::ConsoleAppender(false, true));
        logger.addAppender(consoleAppender);
        return logger;
    }

}
