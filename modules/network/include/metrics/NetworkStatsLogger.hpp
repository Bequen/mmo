#pragma once

#include <cstdint>
#include <chrono>
#include <filesystem>
#include <cassert>

#include "Address.hpp"
#include "BucketMetric.hpp"
#include "packets/Packet.hpp"
#include "MessageRegistry.hpp"

namespace tw::net {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

struct NetworkSendInfo {
    PacketType message_type;
    bool is_sent_by_us;
    Address target;
    TimePoint timepoint;
    std::span<uint8_t> buffer;

    NetworkSendInfo(
            PacketType message_type,
            bool is_sent_by_us,
            const Address& target,
            const std::span<uint8_t> buffer
    ) :
        message_type(message_type),
        is_sent_by_us(is_sent_by_us),
        target(target),
        timepoint(std::chrono::steady_clock::now()),
        buffer(buffer)
    {
    }
};

class NetworkStatsLogger {
private:


    std::vector<NetworkSendInfo> m_backlog;

    std::vector<uint8_t> m_buffer;
    size_t m_left, m_right;

    std::optional<std::ostream> m_output;

    using Interval = std::chrono::seconds;

    BucketMetric<uint32_t, Interval, AverageOp<uint32_t>> m_ping_metric;
    BucketMetric<uint32_t, Interval, SumOp<uint32_t>> m_outgoing;
    BucketMetric<uint32_t, Interval, SumOp<uint32_t>> m_incoming;

public:

    NetworkStatsLogger() :
        m_backlog(10000, {MESSAGE_PACKET, false, Address({}, 0), {}}),
        m_buffer(1000000),
        m_left(0), m_right(0),
        m_ping_metric("ms", 1000),
        m_outgoing("b/s", 1000),
        m_incoming("b/s", 1000)
    { }

    void set_file_output(std::filesystem::path path);

    size_t get_size() {
        return m_right - m_left + (m_right < m_left ? m_backlog.size() : 0);
    }

    NetworkSendInfo& get_item(uint32_t idx) {
        return m_backlog[(m_left + idx) % m_backlog.size()];
    }

    std::span<uint8_t> allocate_memory_for_buffer(size_t size) {
        uint32_t start = m_right;
        if(m_buffer.size() - m_right < size) {
            // throw away packets from the start to make space
            for(; m_backlog[m_left].buffer.data() < m_buffer.data() + start + size &&
                    m_left != m_right; m_left = (m_left + 1) % m_buffer.size()) { }

            start = 0;
        }

        uint32_t end = start + size;
        return std::span<uint8_t>(m_buffer.begin() + start, m_buffer.begin() + end);
    }

    // constexpr void log(PacketType message_type, bool is_sent, const Address& target, const ByteBuffer& content) {
    //     std::span<uint8_t> span = allocate_memory_for_buffer(content.size());

    //     memcpy(span.data(), content.data().data(), content.size());

    //     m_right = (m_right + 1) % m_backlog.size();
    //     m_backlog[m_right] = NetworkSendInfo(message_type, is_sent, target, span);
    // }

    // constexpr void log_receive(
    //         PacketType message_type,
    //         const Address& from
    // ) {
    //     log(message_type, false, from, content);
    //     m_incoming.push(content.size());
    // }

    // constexpr void log_send(
    //         PacketType message_type,
    //         const Address& to
    // ) {
    //     log(message_type, true, to, content);
    //     m_outgoing.push(content.size());
    // }

    void log_ping(uint32_t ping) {
        m_ping_metric.push(ping);
    }

    BucketMetric<uint32_t, Interval, AverageOp<uint32_t>>& ping(){
        return m_ping_metric;
    }

    BucketMetric<uint32_t, Interval>& outgoing(){
        return m_outgoing;
    }

    BucketMetric<uint32_t, Interval>& incoming(){
        return m_incoming;
    }
};

}
