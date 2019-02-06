#include "regular-message.h"

#include <ctime>

namespace proto {

    regular_message::regular_message(payload_t payload)
        : base_message(message_type::regular)
        , payload_{payload}
    { }

    regular_message::regular_message(bytes data)
        : base_message(std::move(data))
    { }

    std::size_t regular_message::message_length() noexcept
    {
        return base_message::message_length() + sizeof(payload_t);
    }

    std::uint32_t regular_message::payload() const noexcept
    {
        return payload_;
    }

    void regular_message::save()
    {
        base_message::save();
        save_uint32(payload_);
    }

    void regular_message::load()
    {
        base_message::load();
        payload_ = load_uint32();
    }

    regular_message make_regular_message()
    {
        std::srand(static_cast<std::uint32_t>(std::time(nullptr)));
        const auto rand_value{static_cast<std::uint32_t>(std::rand())};
        return make_message<regular_message>(rand_value);
    }

}
