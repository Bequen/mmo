#pragma once

#include "NetworkError.hpp"
#include "tl/expected.hpp"
#include <span>

namespace tw::net {

template<typename T>
class Read {
public:
    virtual ~Read() = default;
    virtual tl::expected<size_t, NetworkError> read_into(std::span<T> target) = 0;

    tl::expected<size_t, NetworkError> read_exact_into(std::span<std::byte> data) {
        size_t total_read = 0;
        while (total_read < data.size()) {
            auto read = this->read_into(data.subspan(total_read));
            if(!read.has_value()) {
                if(read.error().m_type == NetworkErrorType::WOULD_BLOCK) {
                    continue;
                } else {
                    return read;
                }
            }

            total_read += read.value();
        }
        return total_read;
    }

    tl::expected<std::vector<std::byte>, NetworkError> read_exact(size_t size) {
        std::vector<std::byte> buffer(size);
        auto result = this->read_exact_into(std::span{buffer});

        if(result.has_value()) {
            return buffer;
        }

        return tl::make_unexpected(result.error());
    }

};

}
