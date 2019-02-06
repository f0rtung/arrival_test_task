#pragma once

#include "../common.h"
#include <common/src/types.h>
#include "../tcp-session/tcp-session.h"

#include <list>

namespace balancer {

    class tcp_server{
    public:
        tcp_server(std::uint16_t port, const route_map &route_map);
        void start();
        void stop();

    private:
        void start_accept(evutil_socket_t socket, const std::string &client_addr);

        template<typename t, typename deleter_t>
        void check_null(const std::unique_ptr<t, deleter_t> &ptr, const std::string &error_msg)
        {
            if(!ptr) {
                log_error_stop_and_throw(error_msg);
            }
        }

        void check_result_code(int result_code, const std::string &error_msg);
        void log_error_stop_and_throw(const std::string &error_msg);

    private:
        const std::uint16_t port_;
        const route_map route_map_;
        log4cplus::Logger logger_;
        common::event_base_ptr eb_;
        common::listener_ptr listener_;
        std::list<std::unique_ptr<session_iface>> sessions_;
    };

}
