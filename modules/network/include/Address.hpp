#pragma once

#include <arpa/inet.h> 
#include <string>
#include <cstring>
#include <optional>
#include <sys/socket.h>
#include <format>

namespace tw::net {

/**
 * IP Address 
 */
struct Address {
    sockaddr_in address;

    Address(const std::optional<std::string>& address, int port) {
        std::memset((char*)&this->address, 0, sizeof(this->address));

        this->address.sin_port = htons(port);
        this->address.sin_addr.s_addr = address.has_value() ? inet_addr(address->c_str()) : INADDR_ANY;
        this->address.sin_family = AF_INET;
    }


    Address(sockaddr_storage& storage) {
        switch(storage.ss_family) {
            case AF_INET:
                this->address = *reinterpret_cast<sockaddr_in*>(&storage);
                break;
        }
    }

    Address(sockaddr_storage&& storage) {
        switch(storage.ss_family) {
            case AF_INET:
                this->address = *reinterpret_cast<sockaddr_in*>(&storage);
                break;
        }
    }

    inline const bool port() const {
        return ntohs(address.sin_port);
    }

    inline const std::string to_string() const {
        uint8_t a = address.sin_addr.s_addr;
        uint8_t b = address.sin_addr.s_addr >> 4;
        uint8_t c = address.sin_addr.s_addr >> 8;
        uint8_t d = address.sin_addr.s_addr >> 12;

        return std::format("{}.{}.{}.{}:{}", a, b, c, d, port());
    }
};

}
