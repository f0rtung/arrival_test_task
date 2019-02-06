#pragma once

#include "../common.h"
#include <common/src/types.h>
#include <logger/src/logger.h>
#include <proto/src/init-message.h>
#include <proto/src/regular-message.h>

#include <functional>

namespace balancer {

    class session_iface {
    public:
        virtual ~session_iface() = default;
        virtual void start() = 0;
        virtual void stop() = 0;
    };

    class tcp_session
        : public session_iface
    {
        using close_op_t = std::function<void()>;

    public:
        tcp_session(event_base *base,
                    evutil_socket_t socket,
                    close_op_t close_op,
                    const route_map &route_map,
                    logger::logger &logger);

        ~tcp_session() override = default;

    public:
        void start() override;
        void stop() override;

    private:
        void start_reading_init_message();
        void read_init_message();
        void start_routing(proto::init_message::client_id_t client_id);
        void connect_to_server(const common::remote_server &server);
        void start_reading_regular_message();
        void read_regular_message();
        void write_regular_message(const proto::bytes &msg);
        void on_next_event(short what);
        static void on_event_cb(bufferevent */*bev*/, short what, void *ctx);
        void check_result_code(int result_code, const std::string &error_msg);

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

        template<typename t, typename deleter_t>
        void check_null(const std::unique_ptr<t, deleter_t> &ptr, const std::string &error_msg)
        {
            if(!ptr) {
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
        const close_op_t close_op_;
        const route_map &route_map_;
        common::bufferevent_ptr client_buffer_;
        common::bufferevent_ptr server_buffer_;
        logger::logger &logger_;
    };

}
