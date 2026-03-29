#pragma once

#include <cstring>
#include <span>
#include <spdlog/spdlog.h>

namespace tw::net {

/**
 * Circular byte buffer.
 */
class ByteBufferReader {
public:
    ByteBufferReader(std::span<std::byte> target) : buffer(target), readOffset(0) {}

    ByteBufferReader(std::span<const std::byte> target) : buffer(target), readOffset(0) {}

    size_t peek_bytes(void* dst, size_t size, size_t offset = 0) {
        if(remaining() - offset < size) {
            return 0;
        }

        size_t cursor = (readOffset + offset) % buffer.size();

        if(cursor + size <= buffer.size()) {
            std::memcpy(dst, buffer.data() + cursor, size);
        } else {
            size_t firstPart = buffer.size() - cursor;
            std::memcpy(dst, buffer.data() + cursor, firstPart);
            std::memcpy((std::byte*)dst + firstPart, buffer.data(), size - firstPart);
        }

        return size;
    }

    template<typename T>
    size_t pop_bytes(T* dst) {
        return pop_bytes(dst, sizeof(T));
    }

    size_t pop_bytes(void* dst, size_t size) {
        size_t peeked = peek_bytes(dst, size);
        if(peeked < size) {
            return 0;
        }

        skip(size);
        return size;
    }

    size_t pop_bytes(std::span<std::byte> dst) {
        return pop_bytes(dst.data(), dst.size());
    }

    size_t remaining() const {
        return buffer.size() - readOffset;
    }

    void reset() {
        readOffset = 0;
    }

    void skip(size_t bytes) {
        readOffset = (readOffset + std::min(bytes, remaining()));
    }

private:
    std::span<const std::byte> buffer;
    size_t readOffset = 0;
};

} // namespace tw::net
