#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace tw::serial {

/**
 * A flat, pre-allocated, non-owning-after-construction byte buffer used as the
 * backing store for BinaryWriter.
 *
 * Design goals:
 *  - Zero heap allocation after the initial reserve().
 *  - reset() in O(1) — just rewinds the write cursor.
 *  - view() gives a read-only span over written bytes, usable directly as a
 *    UDP payload without any copy.
 *  - Not thread-safe: one buffer per client, reused every frame.
 */
class BinaryBuffer {
    std::vector<std::byte> m_storage;
    std::size_t            m_pos{ 0 };

public:
    BinaryBuffer() = default;

    explicit BinaryBuffer(std::size_t capacity) {
        m_storage.resize(capacity);
    }

    // Reserve at least `capacity` bytes.  May only grow, never shrinks.
    void reserve(std::size_t capacity) {
        if (capacity > m_storage.size()) {
            m_storage.resize(capacity);
        }
    }

    // Reset write cursor to 0.  Does NOT zero memory — intentional for perf.
    void reset() noexcept { m_pos = 0; }

    std::size_t size()     const noexcept { return m_pos; }
    std::size_t capacity() const noexcept { return m_storage.size(); }
    bool        empty()    const noexcept { return m_pos == 0; }

    // Returns a view over bytes written so far.  Valid until the next write or reset.
    std::span<const std::byte> view() const noexcept {
        return { m_storage.data(), m_pos };
    }

    // Returns a mutable span over bytes written so far.
    std::span<std::byte> mutable_view() noexcept {
        return { m_storage.data(), m_pos };
    }

    /**
     * Append `n` raw bytes from `src`.
     * Asserts in debug; in release callers must pre-reserve enough space.
     */
    void append(const void* src, std::size_t n) noexcept {
        assert(m_pos + n <= m_storage.size() && "BinaryBuffer overflow — call reserve() with a larger capacity");
        std::memcpy(m_storage.data() + m_pos, src, n);
        m_pos += n;
    }

    /**
     * Reserve `n` bytes in-place and return a pointer to them.
     * Lets callers write directly (e.g. via placement algorithms) without a
     * temporary.
     */
    std::byte* claim(std::size_t n) noexcept {
        assert(m_pos + n <= m_storage.size() && "BinaryBuffer overflow — call reserve() with a larger capacity");
        std::byte* ptr = m_storage.data() + m_pos;
        m_pos += n;
        return ptr;
    }

    /**
     * Patch a previously written 4-byte field at `offset`.
     * Useful for writing a length prefix before the payload is known.
     */
    void patch_u32(std::size_t offset, uint32_t value) noexcept {
        assert(offset + sizeof(uint32_t) <= m_pos);
        std::memcpy(m_storage.data() + offset, &value, sizeof(uint32_t));
    }
};

} // namespace tw::serial
