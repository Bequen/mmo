#include "TcpSocket.hpp"

#include <cerrno>
#include <stdexcept>

#include <print>
#include <sys/socket.h>
#include <netinet/in.h>

namespace tw::net {

TcpSocket::TcpSocket() {
    const int domain = AF_INET;

    socket_fd = socket(domain, SOCK_STREAM, 0);
    if(socket_fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags & O_NONBLOCK) {
        fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    timeval socketTimeout;
    socketTimeout.tv_sec = 10;
    socketTimeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &socketTimeout, sizeof(socketTimeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &socketTimeout, sizeof(socketTimeout));
}

std::optional<TcpSocket> TcpSocket::listen() {
    int32_t result = ::listen(socket_fd, 10);
    if(result < 0) {
        return {};
    }

    sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int their_socket_fd = ::accept(socket_fd, (sockaddr*)&their_addr, &addr_size);
    if(their_socket_fd < 0) {
        return {};
    }

    return TcpSocket{their_socket_fd, Address{their_addr}};
}


}
