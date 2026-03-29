#pragma once

#include "bytebuffer/ByteBuffer.hpp"
#include "bytebuffer/ByteBufferWriter.hpp"
#include "protocol/quicr/QuicrFrame.hpp"
#include "protocol/quicr/QuicrPacket.hpp"

#include <cstddef>

namespace tw::net::quicr {

class QuicrConnection;

class QuicrEncoder {
public:
    static size_t encode_frame(RingByteBuffer& target, QuicrFrame& frame);
};

class QuicrDecoder {
public:
    static QuicrPacket decode_packet(std::span<std::byte> data);
};

class QuicrPacketEncoder {
public:
    QuicrPacketEncoder(std::span<std::byte> target, size_t& offset, QuicrPacketType type, QuicrConnection& connection);

    QuicrPacketEncoder& encode_frame(QuicrFrame& frame);

private:
    std::span<std::byte> m_target;
    ByteBufferWriter m_writer;

    size_t& m_offset;
    QuicrPacketType m_type;
    QuicrConnection& m_connection;
};

}
