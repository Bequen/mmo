#include "protocol/quicr/QuicrEncoder.hpp"
#include "bytebuffer/ByteBufferReader.hpp"
#include "frames/Frame.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"
#include <spdlog/spdlog.h>

namespace tw::net::quicr {

QuicrPacket QuicrDecoder::decode_packet(std::span<std::byte> data) {
    ByteBufferReader reader(data);

    QuicrPacket packet;

    reader.pop_bytes(&packet.type);

    reader.pop_bytes(&packet.destination_id);
    reader.pop_bytes(&packet.local_id);

    reader.pop_bytes(&packet.require_ack);

    if(packet.require_ack) {
        uint32_t packet_number = 0;
        reader.pop_bytes(&packet_number);
        packet.packet_number = packet_number;
    }

    uint32_t length = 0;
    reader.pop_bytes(&length);

    while(reader.remaining()) {
        FrameType frame_type;
        reader.pop_bytes(&frame_type);
        switch(frame_type) {
            case FrameType::KeepAlive:
            case FrameType::Padding: {
                break;
            }

            case FrameType::Ack: {
                uint32_t num_acks = 0;
                reader.pop_bytes(&num_acks);

                std::vector<uint32_t> acked_packets(num_acks);
                reader.pop_bytes(acked_packets.data(), acked_packets.size() * sizeof(uint32_t));

                packet.frames.push_back(QuicrFrame::make_ack(acked_packets));
                break;
            }

            case FrameType::Hello: {
                packet.frames.push_back(QuicrFrame::make_hello());
                break;
            }

            default: {
                spdlog::error("Unrecognized frame type: {}", (uint8_t)frame_type);
                break;
            }
        }
    }

    return packet;
}


QuicrPacketEncoder::QuicrPacketEncoder(std::span<std::byte> target, size_t& offset, QuicrPacketType type, QuicrConnection& connection)
    : m_target(target), m_offset(offset), m_type(type), m_connection(connection), m_writer(m_target) {
}

QuicrPacketEncoder& QuicrPacketEncoder::encode_frame(QuicrFrame& frame) {
    m_writer.write_bytes(&frame.type);
    switch(frame.type) {
        case FrameType::KeepAlive:
        case FrameType::Padding:
        case FrameType::Hello:
            break;
        case FrameType::StreamBase: {
            m_writer.write_bytes(frame.content);
            break;
        }
        case FrameType::Ack:
        case FrameType::AckEcn:
        case FrameType::ResetStream:
        case FrameType::StopSending:
        case FrameType::Crypto:
        case FrameType::NewToken:
        case FrameType::HandshakeDone:
            m_writer.write_bytes(frame.content);
            break;

        default: {
            break;
        }
    }

    return *this;
}

}
