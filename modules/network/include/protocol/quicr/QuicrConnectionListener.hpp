#pragma once

#include "NetworkError.hpp"
#include "tl/expected.hpp"

#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <deque>

namespace tw::net::quicr {

class QuicrConnection;
class QuicrEndpoint;

class QuicrConnectionListener {
    const int PROTOCOL_VERSION = 1;

    QuicrEndpoint* m_endpoint;

    std::deque<QuicrConnection*> m_listened_connections;

    QuicrConnectionListener(QuicrEndpoint* endpoint);

public:
    QuicrConnectionListener(QuicrConnectionListener&& other);

    QuicrConnectionListener& operator=(QuicrConnectionListener&& other) {
        if (this != &other) {
            m_endpoint = other.m_endpoint;
            m_listened_connections = std::move(other.m_listened_connections);
            other.m_endpoint = nullptr;
        }
        return *this;
    }

    static tl::expected<QuicrConnectionListener, NetworkError>
    listen(QuicrEndpoint* endpoint);

    QuicrConnection* listen();

    void on_new_connection(QuicrConnection* connection) {
        m_listened_connections.push_back(connection);
    }

    /**
     * Receives single datagram.
     */
    // tl::expected<size_t, NetworkError> recv_into(std::span<std::byte> buffer, Address* from) {
    //     struct sockaddr_storage sockaddr_from;
    //     socklen_t from_length = sizeof( sockaddr_from );

    //     int result = ::recvfrom(m_socket_fd, (char*)m_input_buffer.data(), m_input_buffer.size(), 0, (struct sockaddr*) &sockaddr_from, &from_length );

    //     *from = Address(sockaddr_from);

    //     return result;
    // }


    // sends single datagram to the given address
    // void send_to(const Address& address, std::span<const std::byte> data) {
    //     size_t r = ::sendto(m_stream.socket_fd(), data.data(), data.size(),
    //              MSG_NOSIGNAL | MSG_DONTWAIT,
    //              address.sockaddr(), address.socklen());

    //     if(r <= 0) {
    //         spdlog::error("Failed to send datagram to {}: {}", address.to_string(), strerror(errno));
    //     }
    // }
};

}
