#include "protocol/quicr/QuicrConnection.hpp"

#include "bytebuffer/ByteBuffer.hpp"
#include "bytebuffer/ByteBufferReader.hpp"
#include "protocol/quicr/QuicrConnectionIdGenerator.hpp"
#include "protocol/quicr/QuicrEncoder.hpp"
#include "protocol/quicr/QuicrFrame.hpp"
#include "protocol/quicr/QuicrPacket.hpp"
#include "protocol/quicr/QuicrPacketType.hpp"
#include "protocol/quicr/VarInt.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"
#include <absl/strings/str_format.h>



namespace tw::net::quicr {

tl::expected<size_t, NetworkError> QuicrConnection::write_datagram(std::span<std::byte> data) {
    if(m_state == QuicrConnectionState::Closed) {
        spdlog::warn("Attempted to write in Closed state");
        return tl::make_unexpected(NetworkError::from_errno(ENOTCONN));
    }

    if(m_last_heartbeat_received < Clock::now() - std::chrono::milliseconds(TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS * 2)) {
        m_state = QuicrConnectionState::Closed;
        return tl::make_unexpected(NetworkError::from_errno(CONNECTION_RESET));
    }

    std::vector<std::byte> dgram;
    dgram.insert(dgram.end(), data.begin(), data.end());

    auto r = m_endpoint->send_to(dgram, m_peer_address);
    if (!r) return tl::make_unexpected(r.error());

    m_last_heartbeat_sent = Clock::now();
    return data.size();
}

void QuicrConnection::send_initial_hello() {
    m_reliability_unit->push_reliable_frame(Clock::now(), QuicrFrame::make_hello());
    m_state = QuicrConnectionState::SentHello;
}


void QuicrConnection::send_hello() {
    std::vector<std::byte> dgram;

    if(m_state != QuicrConnectionState::Closed && m_state != QuicrConnectionState::SentHello) {
        spdlog::warn("Attempted to send Hello in state {}, expected Closed", (int)m_state);
        return;
    }

    VarInt(peer_id()).encode(dgram);
    VarInt(self_id()).encode(dgram);
    VarInt(FrameType::Hello).encode(dgram);
    VarInt(0).encode(dgram);
    VarInt(PROTOCOL_VERSION).encode(dgram);
    VarInt(self_id()).encode(dgram);

    m_state = QuicrConnectionState::SentHello;
    auto r = write_datagram(dgram);
    if(!r) {
        spdlog::error("Failed to send HelloAck: {}", r.error().message());
    }
}

bool QuicrConnection::process_hello(const QuicrPacket& packet, const QuicrFrame& frame) {
    // auto frame_number_v = VarInt::decode(dgram.subspan(off));
    // if (!frame_number_v) return false;
    // uint64_t frame_number = frame_number_v->value;
    // off += frame_number_v->bytes;

    // auto versionV = VarInt::decode(dgram.subspan(off));
    // if (!versionV) return false;
    // uint64_t peer_version = versionV->value;
    // off += versionV->bytes;

    // auto peer_connection_id_v = VarInt::decode(dgram.subspan(off));
    // if (!peer_connection_id_v) return false;
    // uint64_t peer_connection_id = peer_connection_id_v->value;
    // off += peer_connection_id_v->bytes;

    // spdlog::info("Processing hello from: {}", peer_connection_id);

    // if(m_state != QuicrConnectionState::Closed) {
    //     spdlog::warn("Received unexpected Hello in state {}, expected Closed", (int)m_state);
    //     return false;
    // }

    // if(peer_version != PROTOCOL_VERSION) {
    //     spdlog::warn("Unsupported protocol version: {}, expected {}", peer_version, PROTOCOL_VERSION);
    //     return false;
    // }

    if(m_state == QuicrConnectionState::Closed) {
        m_peer_id = packet.local_id;
        m_state = QuicrConnectionState::ReceivedHello;
        m_reliability_unit->push_reliable_frame(Clock::now(), QuicrFrame::make_hello());
    } else if(m_state == QuicrConnectionState::SentHello) {
        m_peer_id = packet.local_id;
        m_state = QuicrConnectionState::Established;
        m_reliability_unit->push_reliable_frame(Clock::now(), QuicrFrame::make_hello_fin());
    }

    return true;
}

bool QuicrConnection::process_hello_fin(const QuicrPacket& packet, const QuicrFrame& frame) {
    if(m_state == QuicrConnectionState::ReceivedHello) {
        m_state = QuicrConnectionState::Established;
        return true;
    }

    return false;
}

void QuicrConnection::send_hello_ack_frame() {
    std::vector<std::byte> dgram;

    VarInt(peer_id()).encode(dgram);
    VarInt(self_id()).encode(dgram);
    VarInt(FrameType::HelloFin).encode(dgram);
    VarInt(PROTOCOL_VERSION).encode(dgram);
    VarInt(self_id()).encode(dgram);
    VarInt(peer_id()).encode(dgram); // acknoledge it's ID

    auto r = write_datagram(dgram);
    if(!r) {
        spdlog::error("Failed to send HelloAck: {}", r.error().message());
    }
}

bool QuicrConnection::process_hello_ack_frame(std::span<const std::byte> dgram, size_t& off) {
    auto peer_version_v = VarInt::decode(dgram.subspan(off));
    if (!peer_version_v) return false;
    uint64_t peer_version = peer_version_v->value;
    off += peer_version_v->bytes;

    auto peer_connection_id_v = VarInt::decode(dgram.subspan(off));
    if (!peer_connection_id_v) return false;
    uint64_t peer_connection_id = peer_connection_id_v->value;
    off += peer_connection_id_v->bytes;

    auto echoed_connection_id_v = VarInt::decode(dgram.subspan(off));
    if (!echoed_connection_id_v) return false;
    uint64_t echoed_connection_id = echoed_connection_id_v->value;
    off += echoed_connection_id_v->bytes;

    if(m_state != QuicrConnectionState::SentHello) {
        spdlog::warn("Received unexpected HelloAck in state {}, expected SentHello", (int)m_state);
        return false;
    }

    if(peer_version != PROTOCOL_VERSION) {
        spdlog::warn("Unsupported protocol version in HelloAck: {}, expected {}", peer_version, PROTOCOL_VERSION);
        return false;
    }

    if(echoed_connection_id != self_id()) {
        spdlog::warn("HelloAck echoed wrong connection ID: {}, expected {}", echoed_connection_id, self_id());
        return false;
    }

    m_peer_id = peer_connection_id;

    send_handshake_done();

    m_state = QuicrConnectionState::Established;

    return true;
}


void QuicrConnection::send_handshake_done() {
    std::vector<std::byte> dgram;

    VarInt(peer_id()).encode(dgram);
    VarInt(self_id()).encode(dgram);
    VarInt(FrameType::HandshakeDone).encode(dgram);
    VarInt(PROTOCOL_VERSION).encode(dgram);
    VarInt(self_id()).encode(dgram);
    VarInt(peer_id()).encode(dgram);

    auto r = write_datagram(dgram);
    if(!r) {
        spdlog::error("Failed to send Handshake Done: {}", r.error().message());
    }
}

bool QuicrConnection::process_handshake_done(std::span<const std::byte> dgram, size_t& off) {
    auto peer_version_v = VarInt::decode(dgram.subspan(off));
    if (!peer_version_v) return false;
    uint64_t peer_version = peer_version_v->value;
    off += peer_version_v->bytes;

    auto peer_connection_id_v = VarInt::decode(dgram.subspan(off));
    if (!peer_connection_id_v) return false;
    uint64_t peer_connection_id = peer_connection_id_v->value;
    off += peer_connection_id_v->bytes;

    auto echoed_connection_id_v = VarInt::decode(dgram.subspan(off));
    if (!echoed_connection_id_v) return false;
    uint64_t echoed_connection_id = echoed_connection_id_v->value;
    off += echoed_connection_id_v->bytes;

    if(peer_version != PROTOCOL_VERSION) {
        spdlog::warn("Unsupported protocol version in HelloAck: {}, expected {}", peer_version, PROTOCOL_VERSION);
        return false;
    }

    if(echoed_connection_id != self_id()) {
        spdlog::warn("HelloAck echoed wrong connection ID: {}, expected {}", echoed_connection_id, self_id());
        return false;
    }

    m_state = QuicrConnectionState::Established;

    return true;
}


tl::expected<void, QuicrError>
QuicrConnection::send_message(std::span<std::byte> data, bool is_reliable) {
    if(state() == QuicrConnectionState::Closed) {
        return tl::make_unexpected(QuicrError(QuicrErrorType::ConnectionClosed));
    }

    m_outbound_messages.emplace_back(data.begin(), data.end());

    return {};

    // std::vector<std::byte> dgram;

    // VarInt(peer_id()).encode(dgram);
    // VarInt(self_id()).encode(dgram);
    // VarInt(FrameType::StreamBase).encode(dgram);
    // VarInt(data.size()).encode(dgram);

    // dgram.insert(dgram.end(), data.begin(), data.end());

    // auto r = write_datagram(dgram);
    // if (!r) {
    //     spdlog::error("Failed to send stream frame: {}", r.error().message());
    //     return false;
    // }

    // return true;
}

bool QuicrConnection::process_stream_frame(uint64_t type, std::span<const std::byte> dgram, size_t& offset) {
    uint32_t length = (uint32_t)dgram.size();

    m_messages.push_back(std::vector<std::byte>(dgram.begin() + offset, dgram.begin() + offset + length));
    offset += length;

    return true;

    // bool has_off = (type & STREAM_FLAG_OFF) != 0;
    // bool has_len = (type & STREAM_FLAG_LEN) != 0;

    // if (has_off) {
    //     auto off_val = VarInt::decode(dgram.subspan(offset));
    //     if (!off_val) return false;
    //     offset += off_val->bytes;
    // }

    // size_t payload_len;
    // // if (has_len) {
    //     auto len_val = VarInt::decode(dgram.subspan(offset));
    //     if (!len_val) return false;
    //     offset += len_val->bytes;
    //     payload_len = len_val->value;

    //     if (offset + payload_len > dgram.size()) return false;
    // // } else {
    // //     payload_len = dgram.size() - offset;
    // // }

    // auto payload = dgram.subspan(offset, payload_len);
    // m_messages.push_back(std::vector<std::byte>(payload.begin(), payload.end()));
    // offset += payload_len;

    return true;
}

tl::expected<void, NetworkError> QuicrConnection::send_keep_alive() {
    std::vector<std::byte> dgram;
    VarInt(FrameType::KeepAlive).encode(dgram);
    VarInt(m_self_id).encode(dgram);

    auto r = m_endpoint->send_to(dgram, m_peer_address);
    if (!r) return tl::make_unexpected(r.error());

    m_last_heartbeat_sent = Clock::now();
    return {};
}

// void QuicrConnection::update() {
//     if(m_state == QuicrConnectionState::Established) {
//         if(m_last_heartbeat_sent < std::chrono::steady_clock::now() - std::chrono::milliseconds(TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS)) {
//             auto r = send_keep_alive();
//             if(!r.has_value()) {
//                 spdlog::error("Failed to send heartbeat - closing connection: {}", r.error().message());
//                 m_state = QuicrConnectionState::Closed;
//             }
//         }

//         if(m_last_heartbeat_received < std::chrono::steady_clock::now() - std::chrono::milliseconds(TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS * 2)) {
//             // Connection is considered lost if we haven't received a heartbeat for twice the interval
//             spdlog::warn("Connection lost due to heartbeat timeout");
//             m_state = QuicrConnectionState::Closed;
//         }
//     }
// }
//
bool QuicrConnection::process_ack_frame(const QuicrPacket& packet, const QuicrFrame& frame) {
    ByteBufferReader reader(std::span(frame.content));

    uint32_t num_acks = frame.content.size() / sizeof(uint32_t);
    // reader.pop_bytes(&num_acks);

    for(int i = 0; i < num_acks; i++) {
        uint32_t acked_packet = 0;
        reader.pop_bytes(&acked_packet);

        m_reliability_unit->on_ack_received(acked_packet);
    }

    return true;
}

void QuicrConnection::process_datagram(std::span<std::byte> dgram) {
    m_last_heartbeat_received = Clock::now();

    QuicrPacket packet = QuicrDecoder::decode_packet(dgram);

    if(packet.require_ack) {
        m_reliability_unit->push_ack(packet.packet_number.value());
    }

    for(auto& frame : packet.frames) {
        switch(frame.type) {
            case FrameType::StreamBase:
            case FrameType::StreamUnreliable: {
                size_t offset = 0;
                process_stream_frame(frame.type, frame.content, offset);
            } break;
            case FrameType::Hello:
                process_hello(packet, frame);
                break;
            case FrameType::HelloFin:
                process_hello_fin(packet, frame);
                break;
            case FrameType::Ack:
                process_ack_frame(packet, frame);
                break;
            // case FrameType::HelloAck:
            //     process_hello_ack_frame(dgram.subspan(offset + packet.header_size), offset);
            //     break;
            // case FrameType::HandshakeDone:
            //     process_handshake_done(dgram.subspan(offset + packet.header_size), offset);
            //     break;
            default:
                spdlog::warn("Unknown frame type: {}", static_cast<int>(frame.type));
                break;
        }
    }

    // auto destination_id_v = VarInt::decode(dgram.subspan(offset));
    // if (!destination_id_v) return;
    // uint64_t destination_id = destination_id_v->value;
    // offset += destination_id_v->bytes;

    // auto source_id_v = VarInt::decode(dgram.subspan(offset));
    // if (!source_id_v) return;
    // uint64_t source_id = source_id_v->value;
    // offset += source_id_v->bytes;

    // m_peer_id = source_id;

    // while (offset < dgram.size()) {
    //     auto typeV = VarInt::decode(dgram.subspan(offset));
    //     if (!typeV) {
    //         spdlog::warn("Failed to decode frame type, dropping rest of datagram");
    //         return;
    //     }
    //     uint64_t t = typeV->value;
    //     offset += typeV->bytes;

    //     // bool is_reliable = *(bool*)(dgram.data() + offset);

    //     // uint64_t packet_number = 0;
    //     // if(is_reliable) {
    //     //     packet_number = VarInt::decode(dgram.subspan(offset))->value;
    //     //     offset += VarInt::decode(dgram.subspan(offset))->bytes;
    //     // }

    //     if (t == FrameType::Padding) {
    //         continue;
    //     }

    //     else if (t == FrameType::KeepAlive) {
    //         continue;
    //     }

    //     else if (t == FrameType::Hello) {
    //         spdlog::info("processing hello");
    //         process_hello(dgram, offset);
    //         continue;
    //     }

    //     else if (t == FrameType::HelloAck) {
    //         if (!process_hello_ack_frame(dgram, offset)) return;
    //         continue;
    //     }

    //     else if (t == FrameType::HandshakeDone) {
    //         if (!process_handshake_done(dgram, offset)) return;
    //         continue;
    //     }

    //     else if (t >= FrameType::StreamBase && t <= (FrameType::StreamBase | 0x07)) {
    //         if (!process_stream_frame(t, dgram, offset)) return;
    //         continue;
    //     }

    //     spdlog::warn("Unknown frame on {} 0x{:x}, dropping rest of datagram", self_id(), t);
    //     return;
    // }
}

// void QuicrConnection::drain_socket() {
//     while (true) {
//         auto r = m_stream.read_into(m_recv_buffer);
//         if (!r || *r == 0) {
//             break;
//         }

//         m_last_heartbeat_received = Clock::now();

//         auto dgram = std::span(m_recv_buffer.data(), *r);

//         size_t offset = 0;
//         auto peer_connection_id_v = VarInt::decode(dgram.subspan(offset));
//         if (!peer_connection_id_v) {
//             spdlog::warn("Failed to decode peer connection ID, dropping datagram");
//             return;
//         }
//         uint64_t peer_connection_id = peer_connection_id_v->value;
//         offset += peer_connection_id_v->bytes;

//         if(peer_connection_id != peer_id()) {
//             spdlog::warn("Received datagram with wrong peer connection ID: {}, expected {}, dropping datagram", peer_connection_id, peer_id());
//             return;
//         }

//         process_datagram(dgram.subspan(offset));
//     }
// }


tl::expected<size_t, NetworkError> QuicrConnection::read_into(std::span<std::byte> target) {
    if(m_messages.empty()) {
        return 0;
    }

    auto& msg = m_messages.front();
    size_t msg_len = msg.size();
    size_t to_copy = std::min(msg_len, target.size());

    std::memcpy(target.data(), msg.data(), to_copy);
    m_messages.pop_front();

    if (to_copy < msg_len) {
        spdlog::warn("Message truncated: {} bytes into {} byte buffer",
            msg_len, target.size());
    }

    return msg_len;  // return full message size so caller knows if truncated
}


void QuicrConnection::on_tick(std::chrono::steady_clock::time_point now) {
    // if (now - m_last_heartbeat_sent > std::chrono::milliseconds(TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS)) {
    //     auto keep_alive_r = send_keep_alive();
    // }

    if(m_last_heartbeat_received < now - std::chrono::milliseconds(TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS * 2)) {

    }
}

bool QuicrConnection::has_next_datagram() {
    if(m_reliability_unit->has_reliable_frames_to_resend()) {
        return true;
    }

    if(m_reliability_unit->has_acks_to_send()) {
        return true;
    }

    if(!m_outbound_messages.empty()) {
        return true;
    }

    return false;
}

std::vector<std::byte> QuicrConnection::pop_datagram() {

    std::vector<std::byte> datagram(64*1024);

    QuicrPacketType type = QuicrPacketType::Initial;
    size_t offset = 0;
    uint32_t packet_number = m_packet_number++;
    QuicrPacketEncoder encoder(datagram, offset, type, packet_number, *this);


    // encode ACK frame
    {
        auto acks = m_reliability_unit->pop_acks_to_send();

        encoder.encode_ack_frame(acks);
    }

    // re-send frames
    {
        // pop already encoded frames
        auto frames_to_resend = m_reliability_unit->pop_frames_to_resend(packet_number);
        for(auto& frame : frames_to_resend) {
            frame.frame_number = packet_number;
            encoder.encode_frame(frame);

            // auto deadline = Clock::now() + std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS);
            // m_reliability_unit->push_reliable_frame_to_send(deadline, std::move(frame));
        }
    }

    while(true) {
        if(m_outbound_messages.empty()) {
            break;
        }

        auto outbound = m_outbound_messages.front();
        m_outbound_messages.pop_front();

        encoder.encode_stream_frame(outbound, true);
    }

    m_last_heartbeat_sent = Clock::now();

    return std::vector<std::byte>(datagram.begin(), datagram.begin() + encoder.size());
}


}
