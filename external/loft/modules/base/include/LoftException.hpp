#pragma once

#include <exception>
#include <string>

namespace lft {

class GpuException : public std::exception {
    std::string m_reason;

public:
    GpuException(const std::string& reason) : m_reason(reason) {
    }

    virtual const char* what() const noexcept override {
        return m_reason.c_str();
    }
};

}
