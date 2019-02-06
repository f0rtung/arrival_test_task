#pragma once

#include <common/src/types.h>
#include <common/src/remote-server.h>
#include <proto/src/base-message.h>

struct bufferevent;

namespace tcp_client {

    class tcp_client {
    public:
        tcp_client(std::uint32_t client_id, const std::string &host,
                   std::uint16_t port, std::uint32_t max_msg_count);
        void start();
        void stop();

    private:
        void write_message(const proto::bytes &msg);
        void on_ready_write();
        void on_next_event(short what);
        bool is_server_finished_read() const noexcept;

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
        log4cplus::Logger logger_;
        const std::uint32_t client_id_;
        const common::remote_server r_server_;
        const std::uint32_t max_msg_count_;
        common::event_base_ptr eb_;
        common::bufferevent_ptr buffer_;
        std::uint32_t curr_msg_number_{0};
    };

}
