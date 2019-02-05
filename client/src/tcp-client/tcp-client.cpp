#include "tcp-client.h"
#include <common/src/types.h>
#include <common/src/utils.h>
#include <proto/src/init-message.h>
#include <proto/src/regular-message.h>

#include <chrono>
#include <cstdlib>
#include <thread>

namespace tcp_client {

    tcp_client::tcp_client(std::uint32_t client_id, const std::string &host, std::uint16_t port)
        : logger_{"tcp_client"}
        , client_id_{client_id}
        , r_server_{host, port}
    { }

    void tcp_client::start()
    {
        logger_.info("Start client with id: ", client_id_,
                     ". Try to send messages to server: ", r_server_);

        auto eb{common::event_base_ptr(event_base_new())};
        check_null(eb, "Can not create new event_base");

        auto buffer{common::bufferevent_ptr(bufferevent_socket_new(eb.get(), -1, BEV_OPT_CLOSE_ON_FREE))};
        check_null(eb, "Can not create new bufferevent");

        const auto on_write = [](bufferevent *bev, void *ctx)
        {
            auto *self{static_cast<tcp_client *>(ctx)};
            self->on_ready_write(bev);
        };

        const auto on_event = [](bufferevent */*bev*/, short what, void *ctx)
        {
            auto *self{static_cast<tcp_client *>(ctx)};
            self->on_next_event(what);
        };

        auto *bev{buffer.get()};
        bufferevent_setcb(bev, nullptr, on_write, on_event, this);
        check_result_code(bufferevent_enable(bev, EV_WRITE), "Can not enable bufferevent for writing");

        const auto init_message{proto::make_init_message(client_id_)};
        write_message(bev, init_message.as_bytes());

        const auto sock{r_server_.sockaddr()};
        const auto connect_result{
            bufferevent_socket_connect(bev, reinterpret_cast<const sockaddr *>(&sock), sizeof(sock))};

        check_result_code(connect_result, "Can not connect to server");
        check_result_code(event_base_dispatch(eb_.get()), "Can not run event loop");
    }

    void tcp_client::write_message(bufferevent *bev, const proto::bytes &msg)
    {
        const auto bev_write_result{bufferevent_write(bev, msg.data(), msg.size())};
        check_result_code(bev_write_result, "Can not write message to bufferevent");
    }

    void tcp_client::on_ready_write(bufferevent *bev)
    {
        const auto regular_message{proto::make_regular_message()};
        logger_.info("Send next message: ", curr_message_number_,
                     " with value: ", regular_message.payload());
        write_message(bev, regular_message.as_bytes());
        if(++curr_message_number_ < max_messages_count_) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        } else {
            logger_.info("Last message was sent");
            check_result_code(bufferevent_disable(bev, EV_WRITE), "Can not disable bufferevent");
        }
    }

    void tcp_client::on_next_event(short what)
    {
        if(what & BEV_EVENT_ERROR) {
            logger_.error("Some error from bufferevent");
        }
    }

    void tcp_client::check_result_code(int result_code, const std::string &error_msg)
    {
        if(-1 == result_code) {
            log_error_and_throw(error_msg);
        }
    }

    void tcp_client::log_error_and_throw(const std::string &error_msg)
    {
        logger_.error(error_msg);
        throw std::runtime_error{error_msg};
    }
}
