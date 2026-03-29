#pragma once

#include "Address.hpp"
#include "NetworkError.hpp"
#include "protocol/quicr/QuicrConnection.hpp"

#include <tl/expected.hpp>
#include <unordered_map>

namespace tw::net::quicr {

class QuicrConnection;
class QuicrConnectionListener;

class QuicrEndpoint {
    int32_t m_socket_fd;
    std::unordered_map<uint64_t, std::unique_ptr<QuicrConnection>> m_connections;

    std::vector<std::byte> m_inbound_buffer;

    QuicrConnectionListener* m_new_connection_handler;

    void process_datagram(std::span<std::byte> datagram, Address from);

    QuicrEndpoint(int socket_fd);


public:

    QuicrEndpoint() : m_socket_fd(-1) { }
    QuicrEndpoint(const QuicrEndpoint&) = delete;
    QuicrEndpoint(QuicrEndpoint&&) = default;
    QuicrEndpoint& operator=(const QuicrEndpoint&) = delete;
    QuicrEndpoint& operator=(QuicrEndpoint&&) = default;

    static tl::expected<QuicrEndpoint, NetworkError> create();

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
