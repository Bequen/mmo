#pragma once

#include "TcpStream.hpp"
#include "bytebuffer/ByteBuffer.hpp"
#include "protocol/quicr/QuicrConnection.hpp"

#include <cstdint>

namespace tw::net {

struct PlayerSession {
public:
    uint32_t session_id;

    quicr::QuicrConnection* quicr_connection;
    // TcpStream tcp_stream;
    // RingByteBuffer inbound_buffer;

    uint32_t last_frame;

    PlayerSession(uint32_t session_id, quicr::QuicrConnection* quicr_connection) :
        session_id(session_id),
        quicr_connection(std::move(quicr_connection)),
        // inbound_buffer(64*1024),
        last_frame(0)
    { }
};

}
