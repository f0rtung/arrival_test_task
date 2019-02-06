#include "../src/regular-message.h"

#include <gtest/gtest.h>

using namespace proto;

TEST(regular_message, RegularMessagePrefix)
{
    // base_message prefix size + sizeof(payload_t)
    ASSERT_EQ(base_message::message_length() + sizeof(regular_message::payload_t),
              regular_message::message_length());
}

TEST(regular_message, SaveToAndLoadFromStream)
{
    const auto msg{make_message<regular_message>(1024)};
    regular_message new_msg{msg.as_bytes()};
    new_msg.load();
    ASSERT_EQ(msg.payload(), new_msg.payload());
}

TEST(regular_message, LoadFromInvalidStream)
{
    bytes data(base_message::message_length() + 1);
    regular_message msg{std::move(data)};
    EXPECT_THROW(msg.load(), std::runtime_error);
}
