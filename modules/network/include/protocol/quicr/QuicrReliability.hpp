#pragma once

#include "bytebuffer/ByteBuffer.hpp"
#include "protocol/quicr/QuicrFrame.hpp"
#include <cstddef>
#include <deque>
#include <set>
#include <vector>

namespace tw::net::quicr {

class QuicrConnection;

/**
 * Assembles next packet from frames.
 */
class QuicrReliabilityUnit {
    using Clock = std::chrono::steady_clock;

    const QuicrConnection* connection;

    std::deque<QuicrFrame> frames;

    std::vector<uint32_t> m_acks_to_send;
    std::vector<std::pair<std::chrono::steady_clock::time_point, QuicrFrame>> awaiting_ack_frames;

    // uint64_t m_largest_received;
    // uint64_t m_ack_bitfield;

    // uint64_t m_frame_number;

    // size_t encode_packet_header(RingByteBuffer& buffer, const QuicrConnection* connection);

    // size_t encode_frame_header(RingByteBuffer& buffer, const QuicrFrame& frame);

    // size_t encode_frame_body(RingByteBuffer& buffer, const QuicrFrame& frame);

    // size_t encode_frame(RingByteBuffer& buffer, const QuicrFrame& frame);

public:
    QuicrReliabilityUnit(const QuicrConnection* connection) :
        connection{connection}
        // m_largest_received{0},
        // m_ack_bitfield{0},
        // m_frame_number{0}
    { }

    void push_ack_received(uint32_t frame_number);

    void push_reliable_frame_received(QuicrFrame frame);

    void push_reliable_frame_to_send(Clock::time_point deadline, QuicrFrame&& frame);
    void push_reliable_frame_to_send(Clock::time_point deadline, QuicrFrame& frame);

    bool has_reliable_frames_to_resend();

    bool has_acks_to_send() {
        return m_acks_to_send.size() > 0;
    }

    std::vector<uint32_t> pop_acks_to_send();

    std::vector<QuicrFrame> pop_frames_to_resend();
};

}
