#pragma once

#include <vector>
#include <string>

namespace proto {

    using byte = std::uint8_t;
    using bytes = std::vector<byte>;

    enum class message_type : std::uint32_t {
        init = 0,
        regular
    };

    /*
     * base_message
     * |*****prefix*****|*****type*****|
     * |                               |
     * |--------message_length---------|
     */

    class base_message {
    public:
        explicit base_message(message_type type);
        explicit base_message(bytes data);
        base_message(base_message &&rhs) = default;

    public:
        message_type type() const noexcept;
        static std::size_t message_length() noexcept;

    public:
        void save();
        void load();
        const bytes &as_bytes() const noexcept;

    protected:
        void save_uint32(std::uint32_t value);
        std::uint32_t load_uint32();

    private:
        void save_message_prefix();

    private:
        message_type type_;
        bytes data_;
        bytes::const_iterator read_pos_;
    };

    template<typename msg_t,
             typename = std::enable_if<std::is_base_of<msg_t, base_message>::value>
             >
    msg_t make_message(std::uint32_t value)
    {
        msg_t msg{value};
        msg.save();
        return msg;
    }

}
