#pragma once

#include "Address.hpp"
#include "NetworkError.hpp"
#include "protocol/quicr/QuicrConnection.hpp"

#include <tl/expected.hpp>
#include <unordered_map>
#include <utility>
#include <unistd.h>

namespace tw::net::quicr {

class QuicrConnection;
class QuicrConnectionListener;

class QuicrEndpoint {
    int32_t m_socket_fd;
    std::unordered_map<uint64_t, std::shared_ptr<QuicrConnection>> m_connections;

    std::vector<std::byte> m_inbound_buffer;

    QuicrConnectionListener* m_new_connection_handler;

    void process_datagram(std::span<std::byte> datagram, Address from);

    QuicrEndpoint(int socket_fd);

    void move_from(QuicrEndpoint&& other) {
        m_socket_fd = std::exchange(other.m_socket_fd, -1);
        m_connections = std::move(other.m_connections);
        m_inbound_buffer = std::move(other.m_inbound_buffer);
        m_new_connection_handler = std::exchange(other.m_new_connection_handler, nullptr);
    }

public:
    QuicrEndpoint() : m_socket_fd(-1) { }
    QuicrEndpoint(const QuicrEndpoint&) = delete;
    QuicrEndpoint(QuicrEndpoint&& other) {
        move_from(std::move(other));
    }

    QuicrEndpoint& operator=(const QuicrEndpoint&) = delete;
    QuicrEndpoint& operator=(QuicrEndpoint&& other) {
        move_from(std::move(other));
        return *this;
    }

    ~QuicrEndpoint() {
        ::close(m_socket_fd);
        m_socket_fd = -1;
    }

    std::vector<std::pair<uint64_t, std::shared_ptr<QuicrConnection>>> clients() const {
        std::vector<std::pair<uint64_t, std::shared_ptr<QuicrConnection>>> result;
        for (const auto& [id, connection] : m_connections) {
            result.emplace_back(id, connection);
        }
        return result;
    }

    static tl::expected<QuicrEndpoint, NetworkError> create();

    /**
     * Creates the QUICr endpoint and binds it to a port.
     */
    static tl::expected<QuicrEndpoint, NetworkError> create_and_bind(int16_t port);

    void assign_listener(QuicrConnectionListener* listener) {
        m_new_connection_handler = listener;
    }

    tl::expected<void, NetworkError> bind(int port);

    tl::expected<QuicrConnection*, NetworkError> connect(Address address);

    tl::expected<size_t, NetworkError> send_to(std::span<std::byte> data, Address to);

    tl::expected<size_t, NetworkError> read_from_into(std::span<std::byte> data, Address* out_from);

    void poll();
};

}
