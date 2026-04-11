#pragma once

#include <algorithm>
#include <string>
namespace tw::net::quicr {

enum class QuicrErrorType {
    ConnectionClosed
};


struct QuicrError {
public:
    QuicrError(QuicrErrorType type) : type_(type), message_(map_quicr_error_type(type)) {}
    QuicrError(QuicrErrorType type, std::string message) : type_(type), message_(std::move(message)) {}
    QuicrErrorType type() const { return type_; }

    std::string message() const { return message_; }

private:
    static std::string map_quicr_error_type(QuicrErrorType type) {
        switch (type) {
            case QuicrErrorType::ConnectionClosed:
                return "ConnectionClosed";
            default:
                return "Unknown";
        }
    }

    std::string message_;
    QuicrErrorType type_;
};

}
