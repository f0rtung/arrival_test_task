#include "tcp-client.h"
#include <common/src/utils.h>
#include <proto/src/init-message.h>
#include <proto/src/regular-message.h>

#include <chrono>
#include <thread>
#include <log4cplus/loggingmacros.h>

namespace tcp_client {

    tcp_client::tcp_client(std::uint32_t client_id, const std::string &host,
                           std::uint16_t port, std::uint32_t max_msg_count)
        : logger_{common::make_logger("tcp_client")}
        , client_id_{client_id}
        , r_server_{host, port}
        , max_msg_count_{max_msg_count}
    { }

    void tcp_client::start()
    {
        LOG4CPLUS_INFO(logger_, "Start client with id: " << client_id_
                       << ". Try to send messages to server: " << r_server_);

        eb_ = common::event_base_ptr(event_base_new());
        check_null(eb_, "Can not create new event_base");

        buffer_ = common::bufferevent_ptr(bufferevent_socket_new(eb_.get(), -1, BEV_OPT_CLOSE_ON_FREE));
        check_null(buffer_, "Can not create new bufferevent");

        const auto on_write = [](bufferevent */*bev*/, void *ctx)
        {
            auto *self{static_cast<tcp_client *>(ctx)};
            self->on_ready_write();
        };

        const auto on_event = [](bufferevent */*bev*/, short what, void *ctx)
        {
            auto *self{static_cast<tcp_client *>(ctx)};
            self->on_next_event(what);
        };

        auto *bev{buffer_.get()};
        bufferevent_setcb(bev, nullptr, on_write, on_event, this);
        check_result_code(bufferevent_enable(bev, EV_WRITE), "Can not enable bufferevent for writing");

        const auto init_message{proto::make_init_message(client_id_)};
        write_message(init_message.as_bytes());

        sockaddr_in sock;
        try {
            sock = r_server_.sockaddr();
        } catch (const std::exception &ex) {
            log_error_stop_and_throw(ex.what());
        }

        const auto connect_result{
            bufferevent_socket_connect(bev, reinterpret_cast<const sockaddr *>(&sock), sizeof(sock))};

        check_result_code(connect_result, "Can not start connection procedure to server");
        check_result_code(event_base_dispatch(eb_.get()), "Can not run event loop");
    }

    void tcp_client::stop()
    {
        if(buffer_) {
            bufferevent_disable(buffer_.get(), EV_WRITE);
            buffer_.reset();
        }

        if(eb_) {
            event_base_loopbreak(eb_.get());
            eb_.reset();
        }
    }

    void tcp_client::write_message(const proto::bytes &msg)
    {
        const auto write_result{bufferevent_write(buffer_.get(), msg.data(), msg.size())};
        check_result_code(write_result, "Can not write message to bufferevent");
    }

    void tcp_client::on_ready_write()
    {
        if(curr_msg_number_++ < max_msg_count_) {
            const auto regular_msg{proto::make_regular_message()};
            LOG4CPLUS_INFO(logger_, "Send next message: " << curr_msg_number_
                           << " with payload: " << regular_msg.payload());
            write_message(regular_msg.as_bytes());
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        } else {
            if(is_server_finished_read()) {
                LOG4CPLUS_INFO(logger_, "Last message was sent");
                stop();
            }
        }
    }

    void tcp_client::on_next_event(short what)
    {
        if(what & BEV_EVENT_ERROR) {
            LOG4CPLUS_ERROR(logger_, "Some error from bufferevent. Stop client");
            stop();
        }
        if(what & BEV_EVENT_CONNECTED) {
            LOG4CPLUS_INFO(logger_, "Connected to server: " << r_server_);
        }
    }

    bool tcp_client::is_server_finished_read() const noexcept
    {
        const auto evbuff{bufferevent_get_output(buffer_.get())};
        return 0 == evbuffer_get_length(evbuff);
    }

    void tcp_client::check_result_code(int result_code, const std::string &error_msg)
    {
        if(-1 == result_code) {
            log_error_stop_and_throw(error_msg);
        }
    }

    void tcp_client::log_error_stop_and_throw (const std::string &error_msg)
    {
        LOG4CPLUS_ERROR(logger_, error_msg);
        stop();
        throw std::runtime_error{error_msg};
    }

}
