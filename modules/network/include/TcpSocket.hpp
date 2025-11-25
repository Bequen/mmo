#pragma once

#include <cerrno>
#include <format>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <print>
#include <utility>

#include "Address.hpp"
#include "ByteBuffer.hpp" 
#include "NetworkResult.hpp"
#include "StatsLogger.hpp"
#include "exception/SocketClosedException.hpp"

namespace tw::net {

class TcpConnection {
    int socket_fd;
    Address address;

public:
    TcpConnection(int socket_fd, Address address) :
        socket_fd(socket_fd),
        address(address)
    {
    }

    ~TcpConnection() {
        if(socket_fd >= 0) {
            std::println("Closing socket");
            close(socket_fd);
        }
    }

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    TcpConnection(TcpConnection&& value) :
        socket_fd(std::exchange(value.socket_fd, -1)),
        address(value.address) {

    }


    void set_non_blocking() {
       if(fcntl(socket_fd, F_SETFL, O_NONBLOCK, 1) == -1) {
           throw std::runtime_error("Failed to set socket as non-blocking");
       }
    }

    TcpConnection& operator=(TcpConnection&& value) {
        std::swap(*this, value);
        return *this;
    }

    void send_bytes(Address& address, const ByteBuffer* buffer);

    uint32_t read_bytes_into(ByteBuffer& target) {
        int length = recv(socket_fd, target.data().data(), target.max_size(), 0);

        if(length == -1) {
            if(errno == EWOULDBLOCK || errno == EAGAIN) {
                return 0;
            } else if(errno == ECONNRESET) {
                throw tw::net::SocketClosedException();
            } else {
                throw std::runtime_error(std::format("Failed to read from socket into buffer because: {}", strerror(errno)));
            }
        }

        target.move_write_head(length);

        return length;
    }

    void send_bytes_from(tw::net::ByteBuffer& mesg) {
        int result = ::send(socket_fd, mesg.data().data(), mesg.size(), 0);
        if(result == -1) {
            if(errno == ECONNRESET) {
                throw tw::net::SocketClosedException();
            }
            throw std::runtime_error("Failed to send buffer: " + std::string(strerror(errno)));
        }

        if(result < mesg.size()) {
            throw std::runtime_error(std::format("Failed to send entire buffer. Sent only {} out of {} bytes.", result, mesg.size()));
        }
    }
};

class TcpConnectionRequest;

class TcpSocket {
private:
    int socket_fd;
    bool m_is_binded;
    std::optional<Address> address;

public:
    const std::optional<Address>& target() {
        return address;
    }

    const int handle() const {
        return socket_fd;
    }

    const bool is_binded() const {
        return m_is_binded;
    }

    const bool is_connected() const {
        return !m_is_binded;
    }

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    TcpSocket(TcpSocket&& other) :
        socket_fd(std::exchange(other.socket_fd, -1)),
        m_is_binded(other.m_is_binded),
        address(other.address)
    {
    }

    TcpSocket& operator=(TcpSocket&& other) {
        this->socket_fd = std::exchange(other.socket_fd, -1);
        this->m_is_binded = other.m_is_binded;
        this->address = other.address;
        return *this;
    }

    TcpSocket();

    TcpSocket(int socket_fd, Address address) :
        socket_fd(socket_fd),
        address(address)
    {
    }

    ~TcpSocket() {
        if(socket_fd >= 0) {
            std::println("Closing server socket");
            close(socket_fd);
        }
    }

    std::optional<TcpSocket> listen();

    void set_non_blocking() {
       if(fcntl(socket_fd, F_SETFL, O_NONBLOCK, 1) == -1) {
           throw std::runtime_error("Failed to set socket as non-blocking");
       }
    }

    void set_blocking() {
       if(fcntl(socket_fd, F_SETFL, O_NONBLOCK, 0) == -1) {
           throw std::runtime_error("Failed to set socket as non-blocking");
       }
    }

    NetworkResult bind(Address address) {
        sockaddr_in socket_addr = address.address;

        if(::bind(socket_fd, (sockaddr*)&socket_addr, sizeof(socket_addr)) < 0) {
            return NetworkResult::from_errno(errno);
        }

        this->address = address;
        this->m_is_binded = true;

        return NetworkResult::ok();
    }

    NetworkResult connect(Address target) {
        std::println("Connecting to {}", target.to_string());
        if(::connect(socket_fd, (sockaddr*)&target.address, sizeof(target.address))) {
            return NetworkResult::from_errno(errno);
        }

        this->address = target; 
        this->m_is_binded = false;

        return NetworkResult::ok();
    }

    NetworkResult read_bytes_into(ByteBuffer& target) {
        int length = recv(socket_fd, target.data().data(), target.max_size(), 0);

        if(length == -1) {
            if(errno == EWOULDBLOCK || errno == EAGAIN) { return NetworkResult::ok(); }

            return NetworkResult::from_errno(errno);
        }

        target.move_write_head(length);

        return NetworkResult::ok();
    }

    NetworkResult send_bytes_from(tw::net::ByteBuffer& mesg) {
        int result = ::send(socket_fd, mesg.data().data(), mesg.size(), MSG_NOSIGNAL);
        if(result == -1) {
            return NetworkResult::from_errno(errno);
        }

        if(result < mesg.size()) {
            // throw std::runtime_error(std::format("Failed to send entire buffer. Sent only {} out of {} bytes.", result, mesg.size()));
        }

        return NetworkResult::ok();

    }
};

}
