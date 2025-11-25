#pragma once

#include "Address.hpp"
#include "ByteBuffer.hpp"
#include "packets/Packet.hpp"
#include <cstring>

namespace tw::dbg::tools {

class SavedPacket {
public:
    PacketType type;
    bool is_from_client;

    net::Address origin;
    net::Address target;

    net::ByteBuffer content;

public:
    SavedPacket() :
        origin({}, 0),
        target({}, 0),
        content(1024)
    {
    }
};

class PacketLogger {
private:
    std::vector<SavedPacket> m_packets;
    uint32_t m_left, m_right;

public:
    inline const std::vector<SavedPacket>& packets() const {
        return m_packets;
    }

    PacketLogger(uint32_t max_packets) :
        m_packets(max_packets) {
    }

    void push(PacketType type, bool is_from_client, 
            net::Address origin, net::Address target,
            net::ByteBuffer& content) {
        SavedPacket& packet = m_packets[m_right];
        packet.type = type;
        packet.is_from_client = is_from_client;
        packet.origin = origin;
        packet.target = target;
        memcpy(packet.content.data().data(), content.data().data(), content.writing_head());
    }
};

}
