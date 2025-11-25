#pragma once

#include <cstring>
#include <exception>

namespace tw::net {

class SocketException : public std::exception {
    int m_errno;

public:

    int errno() {
        return m_errno;
    }

    SocketException() {
    }

    SocketException(int errno) :
        m_errno(errno)
    { }

    const char* what() const noexcept override {
        return std::strerror(m_errno);
    }
};

}
