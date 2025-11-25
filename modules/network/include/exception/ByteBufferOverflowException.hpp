#pragma once

#include <exception>

namespace tw::net {

class ByteBufferOverflowException : public std::exception {
    const char* what() const noexcept override {
        return "Byte buffer overflow";
    }
};

}
