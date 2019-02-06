#include "utils.h"
#include "../src/base-message.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>

using namespace proto;

const std::string prefix{"msg#"};

TEST(base_message, BaseMessagePrefix)
{
    // string prefix size + sizeof(message_type)
    ASSERT_EQ(prefix.size() +sizeof(message_type),
              base_message::message_length());
}

TEST(base_message, CheckMessageType)
{
    foreach_message_type([](message_type type){
        base_message msg{type};
        EXPECT_EQ(type, msg.type());
    });
}

TEST(base_message, SaveToStream)
{
    foreach_message_type([](message_type type){
        base_message msg{type};
        msg.save();
        const std::uint32_t type_value{htonl(static_cast<std::uint32_t>(type))};
        const auto *start_pos{reinterpret_cast<const byte *>(&type_value)};
        bytes expected_bytes;
        expected_bytes.reserve(prefix.size() + sizeof(message_type));
        std::copy(prefix.cbegin(), prefix.cend(), std::back_inserter(expected_bytes));
        std::copy(start_pos, start_pos + sizeof(message_type), std::back_inserter(expected_bytes));
        EXPECT_EQ(expected_bytes, msg.as_bytes());
    });
}

TEST(base_message, LoadFromStream)
{
    foreach_message_type([](message_type type){
        base_message msg{type};
        msg.save();
        base_message new_msg{msg.as_bytes()};
        new_msg.load();
        EXPECT_EQ(type, new_msg.type());
    });
}

TEST(base_message, LoadFromInvalidStream)
{
    bytes bytes{1,2,3,4};
    base_message msg{std::move(bytes)};
    EXPECT_THROW(msg.load(), std::runtime_error);
}
