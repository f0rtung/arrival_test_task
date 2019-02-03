#include "tcp-client.h"

#include <cstring>
#include <chrono>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <event2/buffer.h>
#include <cstdlib>
#include <thread>

namespace {

    sockaddr_in fill_sockaddr(const std::string &host, std::uint16_t port)
    {
        sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &sin.sin_addr.s_addr);
        return sin;
    }

}

namespace tcp_client {

    using event_base_ptr = std::unique_ptr<event_base, decltype(&event_base_free)>;
    using bufferevent_ptr = std::unique_ptr<bufferevent, decltype(&bufferevent_free)>;

    tcp_client::tcp_client(std::uint32_t client_id, const std::string &host, std::uint16_t port)
        : logger_{"tcp_client"}
        , client_id_{client_id}
        , host_{host}
        , port_{port}
    { }

    void tcp_client::start()
    {
        logger_.info("Start client with id: ", client_id_,
                     ". Try to send messages to server: ", host_, ":", port_);

        auto eb{event_base_ptr(event_base_new(), &event_base_free)};
        check_null(eb, "Can not create new event_base");

        auto buffer{bufferevent_ptr(bufferevent_socket_new(eb.get(), -1, BEV_OPT_CLOSE_ON_FREE),
                                                           &bufferevent_free)};
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
        check_libevent_result_code(bufferevent_enable(bev, EV_WRITE),
                                   "Can not enable bufferevent for writing");

        const auto init_message{make_init_message()};
        write_message(bev, init_message.as_string());

        const auto sock{fill_sockaddr(host_, port_)};
        const auto connect_result{
            bufferevent_socket_connect(bev, reinterpret_cast<const sockaddr *>(&sock), sizeof(sock))};

        check_libevent_result_code(connect_result, "Can not connect to server");
        check_libevent_result_code(event_base_dispatch(eb.get()), "Can not run event loop");
    }

    void tcp_client::write_message(bufferevent *bev, const std::string &msg)
    {
        const auto bev_write_result{bufferevent_write(bev, msg.data(), msg.size())};
        check_libevent_result_code(bev_write_result, "Can not write message to bufferevent");
    }

    void tcp_client::on_ready_write(bufferevent *bev)
    {
        logger_.info("Send next message: ", curr_message_number_);
        const auto rand_value{static_cast<std::uint32_t>(std::rand())};
        const auto regular_message{make_regular_message(rand_value)};
        write_message(bev, regular_message.as_string());
        if(++curr_message_number_ < max_messages_count_) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        } else {
            logger_.info("Last message was sent");
            check_libevent_result_code(bufferevent_disable(bev, EV_WRITE),
                                       "Can not disable bufferevent");
        }
    }

    void tcp_client::on_next_event(short what)
    {
        if(what & BEV_EVENT_ERROR) {
            logger_.error("Some error from bufferevent");
        }
    }

    void tcp_client::check_libevent_result_code(int result_code, const std::string &error_msg)
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

    proto::init_message tcp_client::make_init_message()
    {
        proto::init_message msg{client_id_};
        msg.save();
        return msg;
    }

    proto::regular_message tcp_client::make_regular_message(std::uint32_t payload)
    {
        proto::regular_message msg{payload};
        msg.save();
        return msg;
    }
}
