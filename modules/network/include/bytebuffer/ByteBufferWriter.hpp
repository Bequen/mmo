#pragma once

#include <cstring>
#include <span>
#include <spdlog/spdlog.h>

namespace tw::net {

/**
 * Circular byte buffer.
 */
class ByteBufferWriter {
public:
    ByteBufferWriter(std::span<std::byte> target) : buffer(target) {}

    template<typename T>
    size_t write_bytes(const T *data) {
        return write_bytes((void*)data, sizeof(T));
    }

    size_t write_bytes(void* data, size_t size) {
        return write_bytes(std::span<const std::byte>{(std::byte*)data, (std::byte*)data + size});
    }

    size_t write_bytes(std::span<const std::byte> data) {
        if(remaining() < data.size()) {
            return 0;
        }

        if(writeOffset + data.size() <= buffer.size()) {
            std::memcpy(buffer.data() + writeOffset, data.data(), data.size());
            writeOffset += data.size();
        } else {
            size_t firstPart = buffer.size() - writeOffset;
            std::memcpy(buffer.data() + writeOffset, data.data(), firstPart);
            std::memcpy(buffer.data(), data.data() + firstPart, data.size() - firstPart);
            writeOffset = (writeOffset + data.size()) % buffer.size();
        }

        return data.size();
    }

    constexpr size_t length() const {
        return writeOffset;
    }

    size_t remaining() const {
        return buffer.size() - writeOffset;
    }

    void reset() {
        writeOffset = 0;
    }

    void skip_write(size_t bytes) {
        writeOffset = (writeOffset + std::min(bytes, remaining()));
    }

private:
    std::span<std::byte> buffer;
    size_t writeOffset = 0;
};

} // namespace tw::net
