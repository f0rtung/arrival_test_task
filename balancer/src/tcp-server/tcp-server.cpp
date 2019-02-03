#include "tcp-server.h"

#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>


namespace {
    std::string address_from_sockaddr(sockaddr *address)
    {
        const auto *addr_in = reinterpret_cast<sockaddr_in *>(address);
        return inet_ntoa(addr_in->sin_addr);
    }

    sockaddr_in fill_sockaddr(std::uint16_t port, std::uint32_t addr)
    {
        sockaddr_in sin;
        memset(&sin, 0, sizeof (sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = htonl(addr);
        return sin;
    }
}

namespace balancer {
    tcp_server::tcp_server(std::uint16_t port)
        : port_{port}
    {}

    void tcp_server::start()
    {
        eb_ = event_base_ptr(event_base_new(), &event_base_free);
        check_null(eb_, "Can not create new event_base");

        const sockaddr_in sin{fill_sockaddr(port_, INADDR_ANY)};

        auto accept_conn_cb{
            [] (evconnlistener */*listener*/, evutil_socket_t socket, sockaddr *address, int /*socklen*/, void *ctx) {
                auto self{static_cast<tcp_server*>(ctx)};
                self->start_accept(socket, address_from_sockaddr(address));
            }
        };

        const auto listener_options{LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE};
        listener_ = listener_ptr(
                    evconnlistener_new_bind(eb_.get(), accept_conn_cb, this, listener_options,
                                            -1, reinterpret_cast<const sockaddr*>(&sin), sizeof(sin)),
                    &evconnlistener_free
                    );
        check_null(listener_, "Can not create new listener");

        logger_.info("Start server");
        if(event_base_dispatch(eb_.get()) < 0) {
            throw std::runtime_error{"Can not run event loop"};
        }
    }

    void tcp_server::start_accept(evutil_socket_t socket, const std::string &client_addr)
    {
        logger_.info("New client connection was accepted, address", client_addr);
        const auto session_it = sessions_.emplace(sessions_.end());
        auto close_op = [this, session_it]() { sessions_.erase(session_it); };
        using session_t = tcp_session<decltype(close_op)>;
        *session_it = std::make_unique<session_t>(eb_.get(), socket, close_op);
        (*session_it)->start();
    }
}