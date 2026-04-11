#pragma once

#include "Address.hpp"
#include "NetworkError.hpp"
#include "bytebuffer/ByteBuffer.hpp"
#include "protocol/quicr/QuicrConnectionIdGenerator.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"
#include "protocol/quicr/QuicrError.hpp"
#include "protocol/quicr/QuicrPacket.hpp"
#include "protocol/quicr/QuicrReliability.hpp"
#include <cstddef>
#include <chrono>
#include <deque>
#include <sys/socket.h>
#include <tl/expected.hpp>

namespace tw::net::quicr {

const int TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS = 5000;
const int TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS = 500;

/**
 * Overwriting ring buffer;
 */
template<typename T>
class Ring {
    std::vector<T> m_buffer;
    size_t m_head = 0;
    size_t m_tail = 0;

public:
    const T pop() {
        T value = m_buffer[m_tail];
        m_tail = (m_tail + 1) % m_buffer.size();
        return value;
    }

    void push_back(T value) {
        m_head = (m_head + 1) % m_buffer.size();

        if(m_tail == m_head) {
            m_tail += 1;
        }

        m_buffer[m_head] = value;
    }

private:
};

class UdpConnectionStreamPayloadQueue {
    std::vector<std::byte> m_buffer;
    Ring<uint32_t> m_payload_ends;

    std::span<std::byte> pop() {
        return std::span(m_buffer.data(), m_payload_ends.pop());
    }
};

enum QuicrConnectionState {
    Closed,
    SentHello,
    ReceivedHello,
    Established
};

constexpr uint64_t STREAM_FLAG_FIN = 0x01;
constexpr uint64_t STREAM_FLAG_LEN = 0x02;
constexpr uint64_t STREAM_FLAG_OFF = 0x04;

class QuicrEndpoint;

/**
 * Established QUICr connection.
 */
class QuicrConnection : Read<std::byte> {
    static constexpr int PROTOCOL_VERSION = 1;
    static constexpr int MAX_HELLO_RETRIES = 5;
    static constexpr int HELLO_RETRY_INTERVAL_MS = 200;

    using Clock = std::chrono::steady_clock;

    Address m_peer_address;
    QuicrEndpoint* m_endpoint;

    QuicrReliabilityUnit* m_reliability_unit;

    uint32_t m_packet_number = 1;

    uint64_t m_self_id;
    uint64_t m_peer_id;

    Clock::time_point m_last_heartbeat_sent;
    Clock::time_point m_last_heartbeat_received;

    QuicrConnectionState m_state;

    std::vector<std::byte> m_recv_buffer;

    std::deque<std::vector<std::byte>> m_messages;

    std::deque<std::vector<std::byte>> m_outbound_messages;

    std::vector<QuicrFrame> m_outbound_frames;

    std::vector<uint32_t> m_hello_packets;

    /**
     * Builds and writes next datagram.
     */
    tl::expected<size_t, NetworkError> write_datagram(std::span<std::byte> data);

public:
    QuicrConnection(uint64_t self_id, uint64_t peer_id, Address peer_address, QuicrEndpoint* endpoint) :
        m_peer_address{peer_address},
        m_endpoint{endpoint},
        m_self_id{generate_id()},
        m_peer_id{generate_id()},
        m_state(QuicrConnectionState::Closed),
        m_last_heartbeat_received(Clock::now()),
        m_recv_buffer(64 * 1024),
        m_reliability_unit(new QuicrReliabilityUnit(this))
    { }

    // static tl::expected<QuicrConnection, NetworkError> connect(const Address& address);

    constexpr Address address() {
        return m_peer_address;
    }

    constexpr const uint64_t& self_id() const {
        return m_self_id;
    }

    constexpr const uint64_t& peer_id() const {
        return m_peer_id;
    }

    constexpr QuicrConnectionState state() {
        return m_state;
    }

    void set_peer_id(uint64_t peer_id) {
        m_peer_id = peer_id;
    }

    bool is_timed_out() const {
        return m_last_heartbeat_received < std::chrono::steady_clock::now() - std::chrono::milliseconds(TW_NET_HEARTBEAT_INTERVAL_IN_MILLIS * 2);
    }

    tl::expected<void, NetworkError> send_keep_alive();

    void send_initial_hello();

    /**
     * Schedules one stream frame to be sent.
     */
    tl::expected<void, QuicrError>
    send_message(std::span<std::byte> data, bool is_reliable);

    /*
     * Processes stream frame and appends message to the queue.
     */
    bool process_stream_frame(uint64_t type, std::span<const std::byte> dgram, size_t& offset);

    /**
     * Hello frame
     * - Protocol version
     * - self connection ID
     */
    void send_hello();

    bool process_hello(const QuicrPacket& packet, const QuicrFrame& frame);

    bool process_hello_fin(const QuicrPacket& packet, const QuicrFrame& frame);

    /**
     * Hello ACK frame:
     * - Protocol version
     * - self connection ID
     * - echoed peer ID
     */
    void send_hello_ack_frame();

    bool process_hello_ack_frame(std::span<const std::byte> dgram, size_t& off);
    /*
     * Handshake Done Frame
     * - Protocol version
     * - self connection ID
     * - echoed peer ID
     */
    void send_handshake_done();

    bool process_handshake_done(std::span<const std::byte> dgram, size_t& off);


    bool process_ack_frame(const QuicrPacket& packet, const QuicrFrame& frame);

    void process_datagram(std::span<std::byte> dgram);

    // void update();

    // void drain_socket();

    tl::expected<size_t, NetworkError> read_into(std::span<std::byte> target) override;

    void on_tick(std::chrono::steady_clock::time_point now);

    bool has_next_datagram();

    std::vector<std::byte> pop_datagram();

    size_t flush() {
        return 0;
    }

    void encode_next_packet(RingByteBuffer& target);
};

}
