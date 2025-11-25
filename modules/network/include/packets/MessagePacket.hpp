#pragma once

#include "Packet.hpp"
#include "ByteBuffer.hpp"

const int MAX_MESG_SIZE = 128;

struct MessagePacket {
    PacketType packet_type;

    uint32_t mesg_size;
    char mesg[MAX_MESG_SIZE];
};

template<>
class Message<MessagePacket> {
public:
    static constexpr PacketType value = MESSAGE_PACKET;
};

template<>
class tw::net::Serializer<MessagePacket> final {
public:
    static bool serialize(ByteBuffer& buffer, MessagePacket& value) {
        buffer.serialize(&value.mesg_size);
        buffer.bytes(value.mesg, value.mesg_size);
        return true;
    }
};
