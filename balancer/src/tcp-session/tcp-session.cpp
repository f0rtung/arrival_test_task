#include "tcp-session.h"

#include <log4cplus/loggingmacros.h>

namespace balancer {

    tcp_session::tcp_session(event_base *base,
                             evutil_socket_t socket,
                             close_op_t close_op,
                             const route_map &route_map,
                             log4cplus::Logger &logger)
        : close_op_{std::move(close_op)}
        , route_map_{route_map}
        , client_buffer_{common::bufferevent_ptr(bufferevent_socket_new(base, socket, BEV_OPT_CLOSE_ON_FREE))}
        , logger_{logger}
    { }

    void tcp_session::start()
    {
        try {
            check_null(client_buffer_, "Invalid client bufferevent");
            LOG4CPLUS_INFO(logger_, "Start new unknown session");
            start_reading_init_message();
        } catch (const std::exception &ex) {
            LOG4CPLUS_ERROR(logger_, "Can not start session with error: " << ex.what());
            stop();
        } catch (...) {
            LOG4CPLUS_ERROR(logger_, "Can not start session with unknown error");
            stop();
        }
    }

    void tcp_session::stop()
    {
        if(client_buffer_) {
            bufferevent_disable(client_buffer_.get(), EV_READ);
            client_buffer_.reset();
        }
        if(server_buffer_) {
            bufferevent_disable(server_buffer_.get(), EV_WRITE);
            server_buffer_.reset();
        }
        close_op_();
    }

    void tcp_session::start_reading_init_message()
    {
        const auto read_cd = [](bufferevent */*buffer*/, void *ctx) {
            auto *self{static_cast<tcp_session *>(ctx)};
            self->read_init_message();
        };
        prepare_client_buffer_for_reading(read_cd, proto::init_message::message_length(), 0);
    }

    void tcp_session::read_init_message()
    {
        const auto msg{read_message<proto::init_message>()};
        if(msg.type() == proto::message_type::init) {
            LOG4CPLUS_INFO(logger_, "We connected with client: " << msg.client_id());
            start_routing(msg.client_id());
        } else {
            const auto type_uint{static_cast<std::uint32_t>(msg.type())};
            LOG4CPLUS_ERROR(logger_, "Invalid init message, type is " << type_uint);
            stop();
        }
    }

    void tcp_session::start_routing(proto::init_message::client_id_t client_id)
    {
        const auto server_it = route_map_.find(client_id);
        if(route_map_.cend() != server_it) {
            try {
                auto &srv{server_it->second};
                connect_to_server(srv);
                LOG4CPLUS_INFO(logger_, "Start routing packets from clietn "
                               << client_id << " to server " << srv);
                start_reading_regular_message();
            } catch (const std::exception &ex) {
                LOG4CPLUS_ERROR(logger_, "Can not start routing, error: " << ex.what());
                stop();
            } catch (...) {
                LOG4CPLUS_ERROR(logger_, "Can not start routing with unknown error");
                stop();
            }
        } else {
            LOG4CPLUS_ERROR(logger_, "Have no information about client: " << client_id);
            stop();
        }
    }

    void tcp_session::connect_to_server(const common::remote_server &server)
    {
        server_buffer_ =
                common::bufferevent_ptr(bufferevent_socket_new(bufferevent_get_base(client_buffer_.get()),
                                                               -1, BEV_OPT_CLOSE_ON_FREE));
        check_null(server_buffer_, "Invalid server bufferevent");

        auto *buff{server_buffer_.get()};
        bufferevent_setcb(buff, nullptr, nullptr, tcp_session::on_event_cb, this);
        check_result_code(bufferevent_enable(buff, EV_WRITE), "Can not enable server bufferevent for reading");

        const auto sock{server.sockaddr()};
        check_result_code(
                    bufferevent_socket_connect(buff,
                                               reinterpret_cast<const sockaddr *>(&sock),
                                               sizeof(sock)),
                    "Can not start connection procedure to server");
    }

    void tcp_session::start_reading_regular_message()
    {
        const auto read_cd = [](bufferevent */*buffer*/, void *ctx) {
            auto *self{static_cast<tcp_session *>(ctx)};
            self->read_regular_message();
        };

        prepare_client_buffer_for_reading(read_cd, proto::regular_message::message_length(), 0);
    }

    void tcp_session::read_regular_message()
    {
        const auto msg{read_message<proto::regular_message>()};
        LOG4CPLUS_INFO(logger_, "Message with payload '" << msg.payload() << "' has been received");
        write_regular_message(msg.as_bytes());
    }

    void tcp_session::write_regular_message(const proto::bytes &msg)
    {
        bufferevent_write(server_buffer_.get(), msg.data(), msg.size());
    }

    void tcp_session::on_next_event(short what)
    {
        if(what & BEV_EVENT_EOF || what & BEV_EVENT_ERROR) {
            LOG4CPLUS_INFO(logger_, "Close session");
            stop();
        }
    }

    void tcp_session::on_event_cb(bufferevent */*bev*/, short what, void *ctx)
    {
        auto *self{static_cast<tcp_session *>(ctx)};
        self->on_next_event(what);
    }

    void tcp_session::check_result_code(int result_code, const std::string &error_msg)
    {
        if(-1 == result_code) {
            throw std::runtime_error{error_msg};
        }
    }

}
