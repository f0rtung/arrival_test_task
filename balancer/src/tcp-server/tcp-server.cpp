#include "tcp-server.h"
#include <common/src/utils.h>

#include <log4cplus/loggingmacros.h>

namespace balancer {

    tcp_server::tcp_server(std::uint16_t port, const route_map &route_map)
        : port_{port}
        , route_map_{route_map}
        , logger_{common::make_logger("tcp_server")}
    { }

    void tcp_server::start()
    {
        eb_ = common::event_base_ptr(event_base_new());
        check_null(eb_, "Can not create new event_base");

        const sockaddr_in sock{common::make_sockaddr(INADDR_ANY, port_)};

        const auto accept_conn_cb{
            [] (evconnlistener */*listener*/, evutil_socket_t socket, sockaddr *address, int /*socklen*/, void *ctx) {
                auto self{static_cast<tcp_server*>(ctx)};
                self->start_accept(socket, common::address_from_sockaddr(address));
            }
        };

        const auto listener_options{LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE};
        listener_ = common::listener_ptr(
                    evconnlistener_new_bind(eb_.get(), accept_conn_cb, this, listener_options,
                                            -1, reinterpret_cast<const sockaddr*>(&sock), sizeof(sock))
                    );
        check_null(listener_, "Can not create new listener");

        LOG4CPLUS_INFO(logger_, "Start server");
        check_result_code(event_base_dispatch(eb_.get()), "Can not run event loop");
    }

    void tcp_server::stop()
    {
        if(listener_) {
            evconnlistener_disable(listener_.get());
            listener_.reset();
        }

        for(const auto &session : sessions_) {
            session->stop();
        }

        if(eb_) {
            event_base_loopbreak(eb_.get());
            eb_.reset();
        }
    }

    void tcp_server::start_accept(evutil_socket_t socket, const std::string &client_addr)
    {
        LOG4CPLUS_INFO(logger_, "New client connection was accepted, client address: " << client_addr);
        const auto session_it{sessions_.emplace(sessions_.end())};
        const auto close_op{[this, session_it]() { sessions_.erase(session_it); }};
        *session_it = std::make_unique<tcp_session>(eb_.get(), socket, close_op, route_map_, logger_);
        (*session_it)->start();
    }

    void tcp_server::check_result_code(int result_code, const std::string &error_msg)
    {
        if(-1 == result_code) {
            log_error_stop_and_throw(error_msg);
        }
    }

    void tcp_server::log_error_stop_and_throw (const std::string &error_msg)
    {
        LOG4CPLUS_ERROR(logger_, error_msg);
        stop();
        throw std::runtime_error{error_msg};
    }

}
