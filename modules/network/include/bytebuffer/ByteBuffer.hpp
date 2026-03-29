#pragma once

#include <cstring>
#include <optional>
#include <span>
#include <spdlog/spdlog.h>
#include <vector>

namespace tw::net {

/**
 * Circular byte buffer.
 */
class RingByteBuffer {
public:
    RingByteBuffer(std::span<std::byte> target, bool is_for_reading) : buffer(target), writeOffset(is_for_reading ? target.size() : 0) {}

    size_t peek_bytes(void* dst, size_t size, size_t offset = 0) {
        if(remaining_read() - offset < size) {
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

    template<typename T>
    size_t write_bytes(const T *data) {
        return write_bytes((void*)data, sizeof(T));
    }

    size_t write_bytes(void* data, size_t size) {
        return write_bytes(std::span<const std::byte>{(std::byte*)data, (std::byte*)data + size});
    }

    size_t write_bytes(std::span<const std::byte> data) {
        if(remaining_write() < data.size()) {
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

    size_t remaining_write() const {
        if(writeOffset >= readOffset) {
            return buffer.size() - writeOffset + readOffset;
        } else {
            return readOffset - writeOffset;
        }
    }

    size_t remaining_read() const {
        if(writeOffset >= readOffset) {
            return writeOffset - readOffset;
        }

        return buffer.size() - readOffset + writeOffset;
    }

    void reset() {
        readOffset = 0;
        writeOffset = 0;
    }

    void skip(size_t bytes) {
        readOffset = (readOffset + std::min(bytes, remaining_read())) % buffer.size();
        spdlog::info("New read offset: {}", readOffset);
    }

    void skip_write(size_t bytes) {
        writeOffset = (writeOffset + std::min(bytes, remaining_write())) % buffer.size();
    }

    std::span<std::byte> get_next_available_block() {
        if (writeOffset >= readOffset) {
            return std::span(buffer.data() + writeOffset, buffer.size() - writeOffset);
        } else {
            return std::span(buffer.data() + writeOffset, readOffset - writeOffset);
        }
    }

private:
    std::span<std::byte> buffer;
    size_t readOffset = 0;
    size_t writeOffset = 0;
};

} // namespace tw::net
