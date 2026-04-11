#pragma once

#include "bytebuffer/ByteBuffer.hpp"
#include "metrics/NetworkStatsLogger.hpp"
#include "protocol/quicr/QuicrFrame.hpp"
#include <cstddef>
#include <deque>
#include <map>
#include <set>
#include <vector>

namespace tw::net::quicr {

class QuicrConnection;

struct QuicrReliablePacket {
public:
    uint32_t packet_number;
    std::set<uint32_t> frame_numbers;
};

struct QuicrReliableFrame {
    Clock::time_point deadline;
    QuicrFrame frame;
};

/**
 * Assembles next packet from frames.
 */
class QuicrReliabilityUnit {
    using Clock = std::chrono::steady_clock;

    const QuicrConnection* connection;

    std::vector<uint32_t> m_acks_to_send;

    std::map<uint32_t, QuicrReliableFrame*> awaiting_ack_frames;
    std::map<uint32_t, QuicrReliablePacket> packets_in_flight;

    uint32_t m_last_frame_number = 0;

    uint32_t next_frame_number() {
        return ++m_last_frame_number;
    }

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

    void on_ack_received(uint32_t frame_number);


    /**
     * Pushes packet to acknowledge
     */
    void push_ack(uint32_t packet_number);

    bool has_acks_to_send() {
        return m_acks_to_send.size() > 0;
    }

    std::vector<uint32_t> pop_acks_to_send();



    void push_reliable_frame(Clock::time_point deadline, QuicrFrame&& frame);
    void push_reliable_frame(Clock::time_point deadline, QuicrFrame& frame);

    bool has_reliable_frames_to_resend();

    /**
     * Pops all frames that should be re-send and marks them with new_packet_number.
     */
    std::vector<QuicrFrame> pop_frames_to_resend(uint32_t new_packet_number);
};

}
