#pragma once

#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <tl/expected.hpp>

#include "Address.hpp"
#include "NetworkResult.hpp"
#include "TcpStream.hpp"

namespace tw::net {

class TcpListener {
    int m_socket_fd;
    Address m_address;
    int m_port;

    TcpListener(int socket_fd, Address address, int port) :
        m_socket_fd(socket_fd),
        m_address(address),
        m_port(port)
    { }

public:
    tl::expected<void, NetworkResult> set_non_blocking() const {
        if(fcntl(m_socket_fd, F_SETFL, O_NONBLOCK, 1) == -1) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        return {};
    }

    TcpListener& operator=(TcpListener&& other) = delete;
    TcpListener& operator=(const TcpListener& other) = delete;

    TcpListener(const TcpListener& other) = delete;

    TcpListener(TcpListener&& other) :
        m_socket_fd(std::exchange(other.m_socket_fd, -1)),
        m_address(other.m_address)
    { }

    constexpr const Address& address() const { return m_address; }

    static tl::expected<TcpListener, NetworkResult> listen(const Address &address, int port) {
        const int domain = AF_INET;
        int socket_fd = socket(domain, SOCK_STREAM, 0);
        if(socket_fd < 0) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        TcpListener listener(socket_fd, address, port);

        if(::bind(listener.m_socket_fd, address.sockaddr(), address.socklen())) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        return std::move(listener);
    }

    tl::expected<TcpStream, NetworkResult> listen() {
        int32_t result = ::listen(m_socket_fd, 10);
        if(result < 0) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        sockaddr_storage their_addr;
        socklen_t addr_size = sizeof(their_addr);
        int their_socket_fd = ::accept(m_socket_fd, (sockaddr*)&their_addr, &addr_size);
        if(their_socket_fd < 0) {
            return tl::make_unexpected(NetworkResult::from_errno(errno));
        }

        spdlog::debug("Accepted connection");

        return TcpStream{their_socket_fd, Address{their_addr}};
    }


};

}
