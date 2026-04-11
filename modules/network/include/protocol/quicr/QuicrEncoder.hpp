#pragma once

#include "bytebuffer/ByteBuffer.hpp"
#include "bytebuffer/ByteBufferReader.hpp"
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
    static QuicrPacket decode_packet_header(std::span<std::byte> data, size_t& offset);

    static QuicrPacket decode_packet(std::span<std::byte> data);
};

template<typename T>
class QuicrFrameCodec {
public:
    static size_t encode(ByteBufferWriter& writer, T& frame);
    static T decode(ByteBufferReader& reader);
};


/*
 * Encodes a QUICr objects into datagram byte vector.
 */
class QuicrPacketEncoder {
public:
    QuicrPacketEncoder(std::span<std::byte> target, size_t& offset,
        QuicrPacketType type, std::optional<uint32_t> packet_number,
        QuicrConnection& connection);

    QuicrPacketEncoder& encode_stream_frame(std::span<std::byte> data, bool is_reliable);

    QuicrPacketEncoder& encode_ack_frame(std::vector<uint32_t>& acked_packets);

    QuicrPacketEncoder& encode_frame(QuicrFrame& frame);

    constexpr size_t size() const {
        return m_writer.length();
    }

private:
    void write_length(size_t value) {
        m_target[size_val_offset] = static_cast<std::byte>(value >> 24);
        m_target[size_val_offset + 1] = static_cast<std::byte>(value >> 16);
        m_target[size_val_offset + 2] = static_cast<std::byte>(value >> 8);
        m_target[size_val_offset + 3] = static_cast<std::byte>(value);
    }

    void set_as_reliable() {
        m_target[is_reliable_val_offset] = static_cast<std::byte>(1);
    }

    std::span<std::byte> m_target;
    ByteBufferWriter m_writer;

    size_t size_val_offset;
    size_t is_reliable_val_offset;

    size_t& m_offset;
    QuicrPacketType m_type;
    QuicrConnection& m_connection;
};

}
