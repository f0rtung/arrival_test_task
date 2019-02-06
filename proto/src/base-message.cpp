#include "base-message.h"

#include <arpa/inet.h>
#include <stdexcept>

namespace {

    const std::string prefix{"msg#"};

}

namespace proto {

    base_message::base_message(message_type type)
        : type_{type}
    { }

    base_message::base_message(bytes data)
        : data_{std::move(data)}
        , read_pos_{std::next(data_.cbegin(), static_cast<bytes::difference_type>(prefix.size()))}
    { }

    void base_message::save()
    {
        save_message_prefix();
        save_uint32(static_cast<std::uint32_t>(type_));
    }

    void base_message::load()
    {
        type_ = static_cast<message_type>(load_uint32());
    }

    const bytes &base_message::as_bytes() const noexcept
    {
        return data_;
    }

    message_type base_message::type() const noexcept
    {
        return type_;
    }

    std::size_t base_message::message_length() noexcept
    {
        return prefix.size() + sizeof(message_type);
    }

    void base_message::save_uint32(std::uint32_t value)
    {
        const std::uint32_t be_value{htonl(value)};
        data_.reserve(data_.size() + sizeof(std::uint32_t));
        const auto *start_pos{reinterpret_cast<const char *>(&be_value)};
        std::copy(start_pos, start_pos + sizeof(std::uint32_t), std::back_inserter(data_));
    }

    std::uint32_t base_message::load_uint32()
    {
        const auto dist{std::distance(read_pos_, data_.cend())};
        const auto uint32_diff_size{static_cast<bytes::difference_type>(sizeof(std::uint32_t))};
        if(dist < uint32_diff_size) {
            throw std::runtime_error{"Can not read uint32: too little data"};
        }
        const std::uint32_t *value_ptr{reinterpret_cast<const std::uint32_t *>(&(*read_pos_))};
        const std::uint32_t value{ntohl(*value_ptr)};
        std::advance(read_pos_, sizeof(std::uint32_t));
        return value;
    }

    void base_message::save_message_prefix()
    {
        data_.reserve(prefix.size());
        std::copy(prefix.cbegin(), prefix.cend(), std::back_inserter(data_));
    }

}
