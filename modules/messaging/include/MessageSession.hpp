#pragma once

#include "Address.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrError.hpp"

#include <tl/expected.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <functional>
#include <span>
#include <unordered_map>
#include <vector>

namespace tw {

// Bidirectional typed messaging layer. Owns its QuicrEndpoint and connection.
// Wire format: [uint32_t type][uint32_t seq][payload bytes]
// seq == 0 means fire-and-forget; non-zero seq correlates a reply to a request().
// Protocol-agnostic: callers are responsible for serialising/deserialising payloads.
// update() polls the endpoint and dispatches inbound data automatically.
class MessageSession {
    static constexpr size_t   MAX_TYPES = 32;
    static constexpr uint32_t SEQ_NONE  = 0;

    struct PendingRequest {
        std::function<void(std::span<const std::byte>)> handler;
        std::chrono::steady_clock::time_point expires_at;
        std::function<void()> on_timeout;
    };

    std::unique_ptr<net::quicr::QuicrEndpoint>    m_endpoint;
    net::quicr::QuicrConnection* m_conn;
    std::vector<std::byte>       m_recv_buf{64 * 1024};
    std::vector<std::function<void(std::span<const std::byte>)>> m_handlers{MAX_TYPES};
    std::unordered_map<uint32_t, PendingRequest> m_pending{};
    uint32_t m_next_seq = 1;

public:
    // Creates a QuicrEndpoint, connects to the given address, and owns both.
    // Throws on failure (via tl::expected::value()).
    explicit MessageSession(net::Address address)
        : m_endpoint(std::make_unique<net::quicr::QuicrEndpoint>(net::quicr::QuicrEndpoint::create().value()))
        , m_conn(m_endpoint->connect(address).value())
    {}

    MessageSession(const MessageSession&) = delete;
    MessageSession& operator=(const MessageSession&) = delete;
    MessageSession(MessageSession&&) = default;
    MessageSession& operator=(MessageSession&&) = default;

    // Register a permanent handler for the given type ID.
    void set_handler(uint32_t type, std::function<void(std::span<const std::byte>)> fn) {
        if (type >= m_handlers.size()) {
            spdlog::warn("MessageSession: type {} exceeds MAX_TYPES", type);
            return;
        }
        m_handlers[type] = std::move(fn);
    }

    // Send a request with a one-shot response handler matched by sequence number.
    // Call update() each game tick to evict timed-out requests.
    tl::expected<void, net::quicr::QuicrError> request(
        uint32_t type,
        std::span<const std::byte> payload,
        std::function<void(std::span<const std::byte>)> on_response,
        std::chrono::milliseconds timeout = std::chrono::seconds(5),
        std::function<void()> on_timeout = nullptr,
        bool reliable = true
    ) {
        const uint32_t seq = m_next_seq++;
        if (m_next_seq == SEQ_NONE) m_next_seq = 1;

        m_pending.emplace(seq, PendingRequest{
            std::move(on_response),
            std::chrono::steady_clock::now() + timeout,
            std::move(on_timeout)
        });
        return send_impl(type, payload, seq, reliable);
    }

    // Expire timed-out pending requests, poll the endpoint, and dispatch any
    // inbound datagrams. Call once per game tick.
    void update() {
        const auto now = std::chrono::steady_clock::now();
        std::erase_if(m_pending, [&](auto& kv) {
            if (kv.second.expires_at > now) return false;
            spdlog::warn("MessageSession: request seq={} timed out", kv.first);
            if (kv.second.on_timeout) kv.second.on_timeout();
            return true;
        });

        m_endpoint->poll();
        while (true) {
            auto r = m_conn->read_into(m_recv_buf);
            if (!r || *r == 0) break;
            dispatch(std::span(m_recv_buf.data(), *r));
        }
    }

    // Decode one framed datagram: [uint32_t type][uint32_t seq][payload].
    // Non-zero seq routes to a pending one-shot handler; seq==0 routes by type.
    void dispatch(std::span<const std::byte> data) {
        constexpr size_t HEADER = sizeof(uint32_t) * 2;
        if (data.size() < HEADER) {
            spdlog::warn("MessageSession: dropped short datagram ({} bytes)", data.size());
            return;
        }
        uint32_t type{}, seq{};
        std::memcpy(&type, data.data(),                    sizeof(type));
        std::memcpy(&seq,  data.data() + sizeof(uint32_t), sizeof(seq));

        const auto payload = data.subspan(HEADER);

        if (seq != SEQ_NONE) {
            if (auto it = m_pending.find(seq); it != m_pending.end()) {
                auto handler = std::move(it->second.handler);
                m_pending.erase(it);
                handler(payload);
                return;
            }
        }

        if (type >= m_handlers.size() || !m_handlers[type]) {
            spdlog::warn("MessageSession: no handler for type {}", type);
            return;
        }
        m_handlers[type](payload);
    }

    // Send a fire-and-forget message.
    tl::expected<void, net::quicr::QuicrError> send(uint32_t type,
                                                     std::span<const std::byte> payload,
                                                     bool reliable = false) {
        return send_impl(type, payload, SEQ_NONE, reliable);
    }

    bool is_established() const {
        return m_conn->state() == net::quicr::QuicrConnectionState::Established;
    }

private:
    tl::expected<void, net::quicr::QuicrError> send_impl(uint32_t type,
                                                          std::span<const std::byte> payload,
                                                          uint32_t seq,
                                                          bool reliable) {
        std::vector<std::byte> buf(sizeof(type) + sizeof(seq) + payload.size());
        std::memcpy(buf.data(),                    &type, sizeof(type));
        std::memcpy(buf.data() + sizeof(type),     &seq,  sizeof(seq));
        std::memcpy(buf.data() + sizeof(type) + sizeof(seq), payload.data(), payload.size());
        return m_conn->send_message(std::span(buf), reliable);
    }
};

} // namespace tw
