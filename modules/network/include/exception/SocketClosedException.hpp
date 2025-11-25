#pragma once

#include <exception>
namespace tw::net {

class SocketClosedException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Socket closed";
    }
};

}
