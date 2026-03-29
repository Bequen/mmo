#pragma once

#include <optional>
#include <span>
#include <spdlog/spdlog.h>

namespace tw::net {

class MessageDeserializer {
public:
    template<typename T>
    static std::optional<T> deserialize(std::span<std::byte> data) {
        T result = {};

        result.ParseFromArray(data.data(), data.size());

        return result;
    }
};

}
