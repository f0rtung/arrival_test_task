#pragma once

#include "../common.h"
#include <common/src/types.h>
#include <logger/src/logger.h>
#include <proto/src/init-message.h>
#include <proto/src/regular-message.h>

namespace balancer {

    class session_iface {
    public:
        virtual ~session_iface() = default;
        virtual void start() = 0;
        virtual void stop() = 0;
    };

    template<typename close_op_t>
    class tcp_session
        : public session_iface
    {

    public:
        tcp_session(event_base *base,
                    evutil_socket_t socket,
                    close_op_t close_op,
                    const route_map &route_map,
                    logger::logger &logger)
            : close_op_{close_op}
            , route_map_{route_map}
            , client_buffer_{common::bufferevent_ptr(bufferevent_socket_new(base, socket, BEV_OPT_CLOSE_ON_FREE))}
            , logger_{logger}
        { }

        ~tcp_session() = default;

        void start() override
        {
            try {
                check_null(client_buffer_, "Invalid client bufferevent");
                logger_.info("Start new unknown session");
                start_reading_init_message();
            } catch (const std::exception &ex) {
                log_error_and_stop("Can not start session with error: ", ex.what());
            } catch (...) {
                log_error_and_stop("Can not start session with unknown error");
            }
        }

        void stop() override
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

    private:
        void start_reading_init_message()
        {
            const auto read_cd = [](bufferevent */*buffer*/, void *ctx) {
                auto *self{static_cast<tcp_session *>(ctx)};
                self->read_init_message();
            };
            prepare_client_buffer_for_reading(read_cd, proto::init_message::message_length(), 0);
        }

        void read_init_message()
        {
            const auto msg{read_message<proto::init_message>()};
            if(msg.type() == proto::message_type::init) {
                logger_.info("We connected with client: ", msg.client_id());
                start_routing(msg.client_id());
            } else {
                const auto type_uint{static_cast<std::uint32_t>(msg.type())};
                log_error_and_stop("Invalid init message, type is ", type_uint);
            }
        }

        void start_routing(proto::init_message::client_id_t client_id)
        {
            const auto server_it = route_map_.find(client_id);
            if(route_map_.cend() != server_it) {
                try {
                    auto &srv{server_it->second};
                    connect_to_server(srv);
                    logger_.info("Start routing packets from clietn ", client_id, " to server ", srv);
                    start_reading_regular_message();
                } catch (const std::exception &ex) {
                    log_error_and_stop("Can not start routing, error: ", ex.what());
                } catch (...) {
                    log_error_and_stop("Can not start routing with unknown error");
                }
            } else {
                log_error_and_stop("Have no information about client: ", client_id);
            }
        }

        void connect_to_server(const common::remote_server &server)
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

        void start_reading_regular_message()
        {
            const auto read_cd = [](bufferevent */*buffer*/, void *ctx) {
                auto *self{static_cast<tcp_session *>(ctx)};
                self->read_regular_message();
            };

            prepare_client_buffer_for_reading(read_cd, proto::regular_message::message_length(), 0);
        }

        void read_regular_message()
        {
            const auto msg{read_message<proto::regular_message>()};
            logger_.info("Message with payload '", msg.payload() ,"' has been received");
            write_regular_message(msg.as_bytes());
        }

        void write_regular_message(const proto::bytes &msg)
        {
            bufferevent_write(server_buffer_.get(), msg.data(), msg.size());
        }

        void on_next_event(short what)
        {
            if(what & BEV_EVENT_EOF || what & BEV_EVENT_ERROR) {
                logger_.info("Close session");
                stop();
            }
        }

        template<typename r_cb_t>
        void prepare_client_buffer_for_reading(const r_cb_t &r_cb, std::size_t lowmark, std::size_t highmark)
        {
            auto *buff{client_buffer_.get()};
            bufferevent_setcb(buff, r_cb, nullptr, tcp_session::on_event_cb, this);
            bufferevent_setwatermark(buff, EV_READ, lowmark, highmark);
            check_result_code(bufferevent_enable(buff, EV_READ),
                              "Can not enable client bufferevent for reading");
        }

        template<typename msg_t>
        msg_t read_message()
        {
            proto::bytes bytes(msg_t::message_length());
            bufferevent_read(client_buffer_.get(), bytes.data(), bytes.size());
            msg_t msg{std::move(bytes)};
            msg.load();
            return msg;
        }

        static void on_event_cb(bufferevent */*bev*/, short what, void *ctx)
        {
            auto *self{static_cast<tcp_session *>(ctx)};
            self->on_next_event(what);
        }

        template<typename t, typename deleter_t>
        void check_null(const std::unique_ptr<t, deleter_t> &ptr, const std::string &error_msg)
        {
            if(!ptr) {
                throw std::runtime_error{error_msg};
            }
        }

        void check_result_code(int result_code, const std::string &error_msg)
        {
            if(-1 == result_code) {
                throw std::runtime_error{error_msg};
            }
        }

        template<typename ...args_t>
        void log_error_and_stop(args_t&& ...args)
        {
            logger_.error(std::forward<args_t>(args)...);
            stop();
        }

    private:
        close_op_t close_op_;
        const route_map &route_map_;
        common::bufferevent_ptr client_buffer_;
        common::bufferevent_ptr server_buffer_;
        logger::logger &logger_;
    };

}
