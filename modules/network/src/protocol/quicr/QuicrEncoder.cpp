#include "protocol/quicr/QuicrEncoder.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "bytebuffer/ByteBufferReader.hpp"
#include "frames/Frame.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"
#include <spdlog/spdlog.h>

namespace tw::net::quicr {

QuicrPacket QuicrDecoder::decode_packet_header(std::span<std::byte> data, size_t &offset) {
    ByteBufferReader reader(data);
    QuicrPacket packet;

    reader.pop_bytes(&packet.type);

    reader.pop_bytes(&packet.destination_id);
    reader.pop_bytes(&packet.local_id);

    reader.pop_bytes(&packet.require_ack);

    // if(packet.require_ack) {
        uint32_t packet_number = 0;
        reader.pop_bytes(&packet_number);
        packet.packet_number = packet_number;
    // }

    // packet.require_ack = false;

    uint32_t length = 0;
    reader.pop_bytes(&length);

    offset = data.size() - reader.remaining();

    return packet;
}

QuicrPacket QuicrDecoder::decode_packet(std::span<std::byte> data) {
    size_t offset = 0;
    QuicrPacket packet = decode_packet_header(data, offset);

    ByteBufferReader reader(data.subspan(offset));

    while(reader.remaining()) {
        FrameType frame_type;
        reader.pop_bytes(&frame_type);
        switch(frame_type) {
            case FrameType::KeepAlive:
            case FrameType::Padding: {
                break;
            }

            case FrameType::Ack: {
                uint8_t num_acks = 0;
                reader.pop_bytes(&num_acks);

                std::vector<uint32_t> acked_packets(num_acks);
                reader.pop_bytes(acked_packets.data(), acked_packets.size() * sizeof(uint32_t));

                packet.frames.push_back(QuicrFrame::make_ack(acked_packets));
                break;
            }

            case FrameType::Hello: {
                packet.require_ack = true;
                packet.frames.push_back(QuicrFrame::make_hello());
                break;
            }

            case FrameType::HelloFin: {
                packet.require_ack = true;
                packet.frames.push_back(QuicrFrame::make_hello_fin());
                break;
            }

            case FrameType::StreamBase:
                packet.require_ack = true;
            case FrameType::StreamUnreliable: {
                uint32_t size = 0;
                reader.pop_bytes(&size);

                std::vector<std::byte> content(size);
                reader.pop_bytes(content.data(), size);
                packet.frames.push_back(QuicrFrame::make_stream(content));
                break;
            }

            default: {
                throw std::runtime_error("Unrecognized frame type: " + std::to_string((uint8_t)frame_type));
                spdlog::error("Unrecognized frame type: {}", (uint8_t)frame_type);
                break;
            }
        }
    }

    return packet;
}


QuicrPacketEncoder::QuicrPacketEncoder(std::span<std::byte> target, size_t& offset,
    QuicrPacketType type, std::optional<uint32_t> packet_number,
    QuicrConnection& connection)
    : m_target(target), m_offset(offset), m_type(type), m_connection(connection), m_writer(m_target),
      size_val_offset(0), is_reliable_val_offset(0) {

        m_writer.write_bytes((uint8_t*)&type);

        m_writer.write_bytes(&m_connection.peer_id());
        m_writer.write_bytes(&m_connection.self_id());

        is_reliable_val_offset = m_writer.length();
        bool require_ack = false; // TODO: When does it need the ACK?
        m_writer.write_bytes(&require_ack);

        //if(require_ack) {
            uint32_t _packet_num = packet_number.value();
            m_writer.write_bytes(&_packet_num);
            //}

        uint32_t length_offset = m_writer.remaining();
        uint32_t length = 0;
        m_writer.write_bytes(&length);
}

QuicrPacketEncoder& QuicrPacketEncoder::encode_stream_frame(std::span<std::byte> data, bool is_reliable) {
    uint8_t frame_type = is_reliable ? FrameType::StreamBase : FrameType::StreamUnreliable;

    if(is_reliable) {
        set_as_reliable();
    }

    m_writer.write_bytes(&frame_type);
    uint32_t length = data.size();
    m_writer.write_bytes(&length);
    m_writer.write_bytes(data);
    return *this;
}

QuicrPacketEncoder& QuicrPacketEncoder::encode_ack_frame(std::vector<uint32_t>& acked_packets) {
    if(!acked_packets.empty()) {
        uint8_t ack_frame_type = FrameType::Ack;
        uint8_t acks_count = acked_packets.size();
        m_writer.write_bytes(&ack_frame_type);
        m_writer.write_bytes(&acks_count);

        for(auto& ack : acked_packets) {
            m_writer.write_bytes(&ack);
        }
    }
    return *this;
}

QuicrPacketEncoder& QuicrPacketEncoder::encode_frame(QuicrFrame& frame) {
    m_offset += m_writer.write_bytes(&frame.type);
    switch(frame.type) {
        case FrameType::KeepAlive:
        case FrameType::Padding:
        case FrameType::Hello:
            set_as_reliable();
            break;
        case FrameType::StreamBase:
            set_as_reliable();
        case FrameType::StreamUnreliable:
        {
            m_offset += m_writer.write_bytes(frame.content);
            break;
        }
        case FrameType::Ack:
        case FrameType::AckEcn:
        case FrameType::ResetStream:
        case FrameType::StopSending:
        case FrameType::Crypto:
        case FrameType::NewToken:
        case FrameType::HandshakeDone:
            set_as_reliable();
            m_offset += m_writer.write_bytes(frame.content);
            break;

        default: {
            break;
        }
    }

    return *this;
}

}
