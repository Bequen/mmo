#include "protocol/quicr/QuicrEndpoint.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEncoder.hpp"
#include "tl/expected.hpp"
#include <chrono>
#include <fcntl.h>
#include <memory>
#include <tracy/Tracy.hpp>

namespace tw::net::quicr {

QuicrEndpoint::QuicrEndpoint(int socket_fd)
    : m_inbound_buffer(64 * 1024), m_socket_fd(socket_fd),
      m_new_connection_handler(nullptr) {

}

tl::expected<QuicrEndpoint, NetworkError> QuicrEndpoint::create() {
    const int domain = AF_INET;
    int socket_fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
    if(socket_fd < 0) {
        spdlog::error("Failed to create socket: {}", strerror(errno));
        return tl::make_unexpected(NetworkError::from_errno(errno));
    }

    if(fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK, 1) == -1) {
        spdlog::error("Failed to set non-blocking mode: {}", strerror(errno));
        return tl::make_unexpected(NetworkError::from_errno(errno));
    }

    return QuicrEndpoint(socket_fd);
}

tl::expected<void, NetworkError> QuicrEndpoint::bind(int port) {
    const int domain = AF_INET;
    struct sockaddr_in addr = {};
    addr.sin_family = domain;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(::bind(m_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        spdlog::error("Failed to bind socket: {}", strerror(errno));
        return tl::make_unexpected(NetworkError::from_errno(errno));
    }

    if(fcntl(m_socket_fd, F_SETFL, fcntl(m_socket_fd, F_GETFL, 0) | O_NONBLOCK, 1) == -1) {
        spdlog::error("Failed to set non-blocking mode: {}", strerror(errno));
        return tl::make_unexpected(NetworkError::from_errno(errno));
    }

    return {};
}

/**
 * Creates new connection from current socket to the address.
 */
tl::expected<QuicrConnection*, NetworkError> QuicrEndpoint::connect(Address address) {
    auto connection = std::make_shared<QuicrConnection>(0, 0, address, this);
    auto inserted_r = m_connections.emplace(connection->self_id(), connection);

    if(!inserted_r.second) {
        return nullptr;
    }

    inserted_r.first->second->send_initial_hello();

    return inserted_r.first->second.get();
}

void QuicrEndpoint::process_datagram(std::span<std::byte> datagram, Address from) {
    ZoneScopedN("Process Datagram");

    // parse first byte as packet type
    if(datagram.size() < 1) {
        return;
    }

    size_t off = 0;

    QuicrPacket packet = QuicrDecoder::decode_packet_header(datagram, off);

    auto connection = m_connections.find(packet.destination_id);

    if(connection == m_connections.end()) {
        auto conn = std::make_shared<QuicrConnection>(0, packet.local_id, from, this);
        auto emplaced = m_connections.emplace(conn->self_id(), conn);
        emplaced.first->second->set_peer_id(packet.local_id);

        emplaced.first->second->process_datagram(datagram);
        m_connections.emplace(packet.destination_id, emplaced.first->second);
        return;
    }

    auto prev_state = connection->second->state();

    connection->second->process_datagram(datagram);

    if(prev_state != QuicrConnectionState::Established && connection->second->state() == QuicrConnectionState::Established) {
        if(m_new_connection_handler != nullptr) {
            m_new_connection_handler->on_new_connection(connection->second.get());
        }
    }
}

tl::expected<size_t, NetworkError> QuicrEndpoint::send_to(std::span<std::byte> data, Address to) {
    size_t total = 0;

    while(total < data.size_bytes()) {
        ssize_t t = ::sendto(m_socket_fd, data.data() + total, data.size() - total, MSG_NOSIGNAL | MSG_DONTWAIT, to.sockaddr(), to.socklen());
        if(t == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            return tl::make_unexpected(NetworkError::from_errno(errno));
        }
        total += t;
    }

    return total;
}

tl::expected<size_t, NetworkError> QuicrEndpoint::read_from_into(std::span<std::byte> data, Address* out_from) {
    struct sockaddr_storage sockaddr_from;
    socklen_t from_length = sizeof( sockaddr_from );

    int read_len = ::recvfrom(m_socket_fd, data.data(), data.size(), 0, (struct sockaddr*)&sockaddr_from, &from_length);
    if(read_len == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        return tl::make_unexpected(NetworkError::from_errno(errno));
    }

    *out_from = std::move(Address(sockaddr_from));

    return read_len;
}

void QuicrEndpoint::poll() {
    while(1) {
        ZoneScopedN("Reading");
        Address address({}, 0);
        auto r = read_from_into(std::span(m_inbound_buffer), &address);
        if(!r || *r == 0) {
            break;
        }

        process_datagram(std::span(m_inbound_buffer).subspan(0, *r), address);
    }

    auto now = std::chrono::steady_clock::now();

    for(auto& connection : m_connections) {
        ZoneScopedN("Per Connection");

        while(connection.second->has_next_datagram()) {
            auto datagram = connection.second->pop_datagram();
            auto send_r = send_to(datagram, connection.second->address());
            if(!send_r) {
                spdlog::error("Failed to send datagram: {}", send_r.error().message());
                break;
            }
        }
    }
}

}
