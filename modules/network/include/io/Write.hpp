#pragma once

#include "NetworkError.hpp"
#include <tl/expected.hpp>
#include <limits>
#include <span>
#include <string>
#include <type_traits>

namespace tw::net {

template<typename T>
class Write {
public:
    virtual ~Write() = default;

    virtual tl::expected<size_t, NetworkError> write(std::span<T> data) = 0;

    tl::expected<size_t, NetworkError> write(const std::string& data) {
        return write(std::span<std::byte>((std::byte*)(data.c_str()), data.size()));
    }

    template<typename TNum,
        typename std::enable_if_t<std::is_integral<TNum>::value || std::is_enum<TNum>::value, bool> = true>
    tl::expected<size_t, NetworkError> write(TNum data) {
        return write(std::as_writable_bytes(std::span{&data, 1}));
    }

    virtual size_t flush() = 0;
};

}
