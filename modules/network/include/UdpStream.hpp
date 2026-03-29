#pragma once

#include <cstddef>
#include <spdlog/spdlog.h>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "Address.hpp"
#include "NetworkError.hpp"
#include "NetworkResult.hpp"
#include "io/Read.hpp"
#include "tl/expected.hpp"
#include "io/Write.hpp"

namespace tw::net {

class UdpStream : public Write<std::byte> {
    int m_socket_fd;
    Address m_address;

public:
    UdpStream(int socket_fd, Address address) :
        m_socket_fd(socket_fd),
        m_address(address)
    { }

public:
    constexpr int socket_fd() const { return m_socket_fd; }

    constexpr const Address& peer_address() const { return m_address; }

    constexpr tl::expected<void, NetworkError> set_non_blocking() const {
        if(fcntl(m_socket_fd, F_SETFL, fcntl(m_socket_fd, F_GETFL, 0) | O_NONBLOCK, 1) == -1) {
            spdlog::error("Failed to set non-blocking mode: {}", strerror(errno));
            return tl::make_unexpected(NetworkError::from_errno(errno));
        }

        return {};
    }

    UdpStream() : m_address({}) {

    }

    UdpStream& operator=(UdpStream&& other) {
        this->m_socket_fd = std::exchange(other.m_socket_fd, -1);
        this->m_address = other.m_address;
        return *this;
    }

    UdpStream& operator=(const UdpStream& other) = delete;

    UdpStream(const UdpStream& other) :
        m_socket_fd(other.m_socket_fd),
        m_address(other.m_address) {

    }

    UdpStream(UdpStream&& other) :
        m_socket_fd(std::exchange(other.m_socket_fd, -1)),
        m_address(other.m_address)
    { }

    static tl::expected<UdpStream, NetworkError> bind(const Address& address) {
        const int domain = AF_INET;
        int socket_fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
        if(socket_fd < 0) {
            return tl::make_unexpected(NetworkError::from_errno(errno));
        }

        if(::bind(socket_fd, address.sockaddr(), address.socklen()) < 0) {
            return tl::make_unexpected(NetworkError::from_errno(errno));
        }

        return UdpStream(socket_fd, Address(address));
    }

    static tl::expected<UdpStream, NetworkResult> to(const Address &address) {
        const int domain = AF_INET;
        int socket_fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
        if(socket_fd < 0) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        UdpStream stream(socket_fd, address);

        // if(::connect(stream.m_socket_fd, (sockaddr*)&address.address, sizeof(address.address))) {
        //     return tl::make_unexpected(NetworkResult::from_errno(errno));
        // }

        return stream;
    }

    static tl::expected<UdpStream, NetworkError> to(int32_t socket_fd, const Address& address) {
        return UdpStream(socket_fd, address);
    }

    tl::expected<size_t, NetworkError> write(std::span<std::byte> data) override {
        size_t total = 0;
        while(total < data.size_bytes()) {
            ssize_t t = ::sendto(m_socket_fd, data.data() + total, data.size() - total, MSG_NOSIGNAL | MSG_DONTWAIT, m_address.sockaddr(), m_address.socklen());
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

    tl::expected<size_t, NetworkError> read_into(std::span<std::byte> data) {
        struct sockaddr_storage sockaddr_from;
        socklen_t from_length = sizeof( sockaddr_from );

        int read_len = ::recvfrom(m_socket_fd, data.data(), data.size(), 0, (struct sockaddr*)&sockaddr_from, &from_length);
        if(read_len == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }

            return tl::make_unexpected(NetworkError::from_errno(errno));
        }

        if(read_len > 0) {
            if(!Address(sockaddr_from).equals(m_address)) {
                spdlog::warn("Received datagram from unexpected address {}, expected {}", Address(sockaddr_from).to_string(), m_address.to_string());
                return 0;
            }
        } else {
            spdlog::warn("Empty datagram from {}", m_address.to_string());
        }

        return read_len;
    }

    tl::expected<size_t, NetworkError> read_into(std::span<std::byte> data, Address* out_from) {
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

    size_t flush() override {
        return 0;
    }
};

}
