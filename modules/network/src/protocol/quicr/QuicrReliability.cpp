#include "protocol/quicr/QuicrReliability.hpp"
#include "bytebuffer/ByteBuffer.hpp"
#include "metrics/NetworkStatsLogger.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include <chrono>
#include <immintrin.h>

namespace tw::net::quicr {

// size_t QuicrReliabilityUnit::encode_packet_header(RingByteBuffer& target, const QuicrConnection* connection) {
//     size_t size = 0;

//     target.write_bytes(&connection->peer_id());
//     target.write_bytes(&connection->self_id());

//     return size;
// }

// size_t QuicrReliabilityUnit::encode_frame_header(RingByteBuffer& buffer, const QuicrFrame& frame) {
//     size_t size = 0;

//     buffer.write_bytes(&frame.type);

//     if(frame.is_reliable) {
//         buffer.write_bytes(&frame.is_reliable);
//         buffer.write_bytes(&frame.frame_number);
//     }

//     return size;
// }

// size_t QuicrReliabilityUnit::encode_frame_body(RingByteBuffer& buffer, const QuicrFrame& frame) {
//     size_t size = 0;


//     return size;
// }

// size_t QuicrReliabilityUnit::encode_frame(RingByteBuffer& buffer, const QuicrFrame& frame) {
//     size_t size = 0;

//     size += encode_frame_header(frame);

//     size += encode_frame_body(frame);

//     return size;
// }

// std::vector<std::byte> QuicrReliabilityUnit::pop_datagram() {
//     size_t size = 0;
//     std::vector<std::byte> datagram;
//     RingByteBuffer byte_buf(datagram);

//     // write header
//     byte_buf.write_bytes(&connection->peer_id());
//     byte_buf.write_bytes(&connection->self_id());

//     size_t last_end = byte_buf.remaining_read();

//     size += encode_packet_header();

//     // resend frames
//     for (const auto& [timestamp, frame] : awaiting_ack_frames) {
//         if(timestamp < std::chrono::steady_clock::now() - std::chrono::seconds(1)) {
//             size += encode_frame(byte_buf, frame);
//             last_end = byte_buf.remaining_read();
//         }
//     }

//     // write body
//     while(size < 1100) {
//         auto frame = frames.front();

//         if(frame.is_reliable) {
//             frame.frame_number = m_frame_number++;
//         }

//         size += encode_frame(byte_buf, frame);
//         frames.pop_front();
//     }

//     return datagram;
// }

// void QuicrReliabilityUnit::process_frame(QuicrFrame frame) {
//     if(frame.is_reliable) {
//         if(m_largest_received == frame.frame_number) {
//             return;
//         }

//         if(m_largest_received < frame.frame_number) {
//             m_ack_bitfield <<= (frame.frame_number - m_largest_received);
//             m_largest_received = frame.frame_number;
//         } else if(m_largest_received > frame.frame_number) {
//             m_ack_bitfield |= (1ULL << (m_largest_received - frame.frame_number));
//         }
//     }
// }
//
bool QuicrReliabilityUnit::has_reliable_frames_to_resend() {
    return std::any_of(awaiting_ack_frames.begin(), awaiting_ack_frames.end(),
        [](const auto& t) { return t.second->deadline < std::chrono::steady_clock::now(); });
}

void QuicrReliabilityUnit::push_ack(uint32_t packet_number) {
    m_acks_to_send.push_back(packet_number);
}

void QuicrReliabilityUnit::on_ack_received(uint32_t packet_number) {
    auto packet = packets_in_flight.find(packet_number);
    if(packet != packets_in_flight.end()) {
        for(auto frame : packet->second.frame_numbers) {
            if(awaiting_ack_frames.erase(frame) == 0) {
                spdlog::error("Failed to erase frame {} from awaiting_ack_frames", frame);
                continue;
            }

            std::erase_if(packets_in_flight, [frame, packet_number](auto& packet) {
                // skip current packet
                if(packet.second.packet_number == packet_number) {
                    return false;
                }

                packet.second.frame_numbers.erase(frame);
                return packet.second.frame_numbers.empty();
            });
        }

        packets_in_flight.erase(packet);
    }



    // m_acks_to_send.push_back(frame_idx);
    // std::erase_if(awaiting_ack_frames,
    //     [frame_idx](const auto& t) {
    //         return t.second.frame_number == frame_idx;
    //     });
}


std::vector<QuicrFrame> QuicrReliabilityUnit::pop_frames_to_resend(uint32_t new_packet_number) {
    std::vector<QuicrFrame> resend_frames;

    auto packet = packets_in_flight.try_emplace(new_packet_number, QuicrReliablePacket{new_packet_number, {}});

    for(auto frame : awaiting_ack_frames) {
        if(frame.second->deadline < Clock::now()) {
            resend_frames.push_back(std::move(frame.second->frame));
            packet.first->second.frame_numbers.insert(frame.first);
            frame.second->deadline = Clock::now() + std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS);
        }
    }

    // std::erase_if(awaiting_ack_frames, [&resend_frames](const auto& item) {
    //     if(item.first < std::chrono::steady_clock::now()) {
    //         resend_frames.push_back(item.second);
    //         return true;
    //     }

    //     return false;
    // });
    return resend_frames;
}

void QuicrReliabilityUnit::push_reliable_frame(Clock::time_point deadline, QuicrFrame&& frame) {
    frame.is_reliable = true;
    frame.frame_number = next_frame_number();
    auto frame_number = frame.frame_number;
    awaiting_ack_frames[frame_number] = new QuicrReliableFrame{deadline, std::move(frame)};

    // awaiting_ack_frames.emplace_back(deadline, frame);
}

void QuicrReliabilityUnit::push_reliable_frame(Clock::time_point deadline, QuicrFrame& frame) {
    frame.is_reliable = true;
    frame.frame_number = next_frame_number();
    auto frame_number = frame.frame_number;
    awaiting_ack_frames[frame_number] = new QuicrReliableFrame{deadline, std::move(frame)};
    // awaiting_ack_frames.emplace_back(deadline, frame);
}

std::vector<uint32_t> QuicrReliabilityUnit::pop_acks_to_send() {
    auto acks = std::vector<uint32_t>(m_acks_to_send);

    m_acks_to_send.clear();

    return acks;
}

}
