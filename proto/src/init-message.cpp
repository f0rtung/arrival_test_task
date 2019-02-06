#include "init-message.h"

namespace proto {

    init_message::init_message(client_id_t client_id)
        : base_message(message_type::init)
        , client_id_{client_id}
    { }

    init_message::init_message(bytes data)
        : base_message(std::move(data))
    { }

    std::size_t init_message::message_length() noexcept
    {
        return base_message::message_length() + sizeof(client_id_t);
    }

    std::uint32_t init_message::client_id() const noexcept
    {
        return client_id_;
    }

    void init_message::save()
    {
        base_message::save();
        save_uint32(client_id_);
    }

    void init_message::load()
    {
        base_message::load();
        client_id_ = load_uint32();
    }

    init_message make_init_message(init_message::client_id_t client_id)
    {
        return make_message<init_message>(client_id);
    }

}
