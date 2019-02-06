#include "../src/init-message.h"

#include <gtest/gtest.h>

using namespace proto;

TEST(init_message, InitMessagePrefix)
{
    // base_message prefix size + sizeof(client_id_t)
    ASSERT_EQ(base_message::message_length() + sizeof(init_message::client_id_t),
              init_message::message_length());
}

TEST(init_message, SaveToAndLoadFromStream)
{
    const auto msg{make_message<init_message>(1024)};
    init_message new_msg{msg.as_bytes()};
    new_msg.load();
    ASSERT_EQ(msg.client_id(), new_msg.client_id());
}

TEST(init_message, LoadFromInvalidStream)
{
    bytes data(base_message::message_length() + 1);
    init_message msg{std::move(data)};
    EXPECT_THROW(msg.load(), std::runtime_error);
}
