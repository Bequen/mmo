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

class TcpStream : public Write<std::byte>, public Read<std::byte> {
    int m_socket_fd;
    Address m_address;

public:
    constexpr int socket_fd() const { return m_socket_fd; }

    constexpr tl::expected<void, NetworkResult> set_non_blocking() const {
        if(fcntl(m_socket_fd, F_SETFL, fcntl(m_socket_fd, F_GETFL, 0) | O_NONBLOCK, 1) == -1) {
            spdlog::error("Failed to set non-blocking mode: {}", strerror(errno));
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        return {};
    }

    TcpStream(int socket_fd, Address address) :
        m_socket_fd(socket_fd),
        m_address(address)
    { }

    TcpStream& operator=(TcpStream&& other) = delete;
    TcpStream& operator=(const TcpStream& other) = delete;

    TcpStream(TcpStream&& other) :
        m_socket_fd(std::exchange(other.m_socket_fd, -1)),
        m_address(other.m_address)
    { }

    static tl::expected<TcpStream, NetworkResult> connect(const Address &address) {
        const int domain = AF_INET;
        int socket_fd = socket(domain, SOCK_STREAM, 0);
        if(socket_fd < 0) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        TcpStream stream(socket_fd, address);

        if(::connect(stream.m_socket_fd, address.sockaddr(), address.socklen())) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        return stream;
    }

    tl::expected<size_t, NetworkError> write(std::span<std::byte> data) override {
        uint32_t num_attempts = 0;
        size_t total = 0;
        while(total < data.size_bytes()) {
            ssize_t t = ::send(m_socket_fd, data.data() + total, data.size() - total, MSG_NOSIGNAL | MSG_DONTWAIT);
            if(t == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    num_attempts++;

                    continue;
                }

                return tl::make_unexpected(NetworkError::from_errno(errno));
            }
            total += t;
        }

        return total;
    }

    tl::expected<size_t, NetworkError> read_into(std::span<std::byte> data) override {
        int read_len = ::recv(m_socket_fd, data.data(), data.size(), 0);
        if(read_len == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }

            return tl::make_unexpected(NetworkError::from_errno(errno));
        }

        return read_len;
    }

    size_t flush() override {
        return 0;
    }
};

}
