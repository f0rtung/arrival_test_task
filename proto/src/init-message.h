#pragma once

#include "base-message.h"

namespace proto {

    /*
     * init_message
     * |*****base_message*****|*****client_id:uint32*****|
     * |                                                 |
     * |-----------------message_length------------------|
     */

    class init_message
        : public base_message
    {
    public:
        using client_id_t = std::uint32_t;

    public:
        explicit init_message(client_id_t clietn_id);
        explicit init_message(bytes data);
        init_message(init_message &&rhs) = default;

    public:
        client_id_t client_id() const noexcept;
        static std::size_t message_length() noexcept;

    public:
        void save();
        void load();

    private:
        client_id_t client_id_;
    };

    init_message make_init_message(init_message::client_id_t client_id);

}
