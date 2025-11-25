#pragma once

#include "Packet.hpp"
#include "ByteBuffer.hpp"

const int MAX_USERNAME_LENGTH = 128;

struct LoginPacket {
    uint32_t username_length;
    char username[MAX_USERNAME_LENGTH];
};

template<>
class Message<LoginPacket> {
public:
    static constexpr PacketType value = LOGIN_PACKET;
};

template<>
class tw::net::Serializer<LoginPacket> final {
public:
    static bool serialize(ByteBuffer& buffer, LoginPacket& value) {
        buffer.serialize(&value.username_length);
        buffer.bytes(value.username, value.username_length);
        return true;
    }
};



struct LoginStatusPacket {
    bool is_okay;

    LoginStatusPacket() {
    }

    LoginStatusPacket(bool is_okay) :
        is_okay(is_okay)
    {
    }
};

template<>
class Message<LoginStatusPacket> {
public:
    static constexpr PacketType value = LOGIN_RESPONSE_PACKET;
};

template<>
class tw::net::Serializer<LoginStatusPacket> final {
public:
    static bool serialize(ByteBuffer& buffer, LoginStatusPacket& value) {
        return buffer.serialize(&value.is_okay);
    }
};
