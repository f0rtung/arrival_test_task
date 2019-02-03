#pragma once

#include <logger/src/logger.h>
#include <proto/src/init-message.h>
#include <proto/src/regular-message.h>

#include <event2/event.h>
#include <event2/bufferevent.h>

namespace tcp_client {

    class tcp_client {
    public:
        tcp_client(std::uint32_t client_id, const std::string &host, std::uint16_t port);
        void start();

    private:
        void write_message(bufferevent *bev, const std::string &msg);
        void on_ready_write(bufferevent *bev);
        void on_next_event(short what);

        template<typename T>
        void check_null(const T &ptr, const std::string &error_msg)
        {
            if(!ptr) {
                log_error_and_throw(error_msg);
            }
        }

        void check_libevent_result_code(int result_code, const std::string &error_msg);
        void log_error_and_throw(const std::string &error_msg);
        proto::init_message make_init_message();
        proto::regular_message make_regular_message(std::uint32_t payload);

    private:
        logger::logger logger_;
        const std::uint32_t client_id_;
        const std::string host_;
        const std::uint16_t port_;
        const std::uint32_t max_messages_count_{1000};
        std::uint32_t curr_message_number_{0};
    };

}
