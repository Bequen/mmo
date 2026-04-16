#pragma once

/**
 * tw::serial — generic, zero-allocation binary codec
 * ====================================================
 *
 * Extensibility model (ADL / explicit specialisation)
 * ---------------------------------------------------
 * To make a type T serialisable, either:
 *
 *   a) Specialise tw::serial::Codec<T>:
 *
 *       template<>
 *       struct tw::serial::Codec<MyType> {
 *           static void encode(BinaryWriter& w, const MyType& v);
 *           static MyType decode(BinaryReader& r);
 *       };
 *
 *   b) Or provide free functions in the same namespace as T:
 *
 *       void tw_serial_encode(BinaryWriter& w, const MyType& v);
 *       MyType tw_serial_decode(BinaryReader& r, std::type_identity<MyType>);
 *
 * The BinaryWriter/BinaryReader forward-declared here are defined in
 * BinaryWriter.hpp / BinaryReader.hpp.  Codec.hpp itself is header-only
 * and has no external dependencies beyond the C++ standard library.
 */

#include "BinaryBuffer.hpp"

#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

namespace tw::serial {

// Forward declarations
class BinaryWriter;
class BinaryReader;

// ── Primary template — intentionally incomplete so missing specialisations
//    produce a clear compiler error rather than silent wrong behaviour.
template<typename T>
struct Codec;

// ── Concept: a type is Encodable if Codec<T>::encode exists.
template<typename T>
concept Encodable = requires(BinaryWriter & w, const T & v) {
    Codec<T>::encode(w, v);
};

// ── Concept: a type is Decodable if Codec<T>::decode exists.
template<typename T>
concept Decodable = requires(BinaryReader & r) {
    { Codec<T>::decode(r) } -> std::same_as<T>;
};

// ──────────────────────────────────────────────────────────────────────────
// BinaryWriter
// ──────────────────────────────────────────────────────────────────────────

/**
 * Thin write-cursor over a BinaryBuffer.
 *
 * All write operations are branch-free memcpy paths for trivially-copyable
 * types.  The buffer itself holds the memory; the writer is just a cursor.
 */
class BinaryWriter {
    BinaryBuffer& m_buf;

public:
    explicit BinaryWriter(BinaryBuffer& buf) noexcept : m_buf(buf) {}

    // ── Raw bytes ────────────────────────────────────────────────────────

    void write_bytes(const void* src, std::size_t n) noexcept {
        m_buf.append(src, n);
    }

    void write_bytes(std::span<const std::byte> data) noexcept {
        m_buf.append(data.data(), data.size());
    }

    // ── Primitive scalar ─────────────────────────────────────────────────

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    void write(const T& value) noexcept {
        m_buf.append(&value, sizeof(T));
    }

    // ── Codec-dispatched write ────────────────────────────────────────────

    template<Encodable T>
    void encode(const T& value) {
        Codec<T>::encode(*this, value);
    }

    // ── Cursor helpers ────────────────────────────────────────────────────

    /** Returns the current write position (useful for length-prefix patching). */
    std::size_t pos() const noexcept { return m_buf.size(); }

    /** Reserve a 4-byte slot at the current position, return its offset. */
    std::size_t reserve_u32() noexcept {
        std::size_t offset = m_buf.size();
        uint32_t placeholder = 0;
        m_buf.append(&placeholder, sizeof(uint32_t));
        return offset;
    }

    /** Patch a 4-byte slot previously reserved with reserve_u32(). */
    void patch_u32(std::size_t offset, uint32_t value) noexcept {
        m_buf.patch_u32(offset, value);
    }

    BinaryBuffer&       buffer()       noexcept { return m_buf; }
    const BinaryBuffer& buffer() const noexcept { return m_buf; }

    void reset() noexcept {
        m_buf.reset();
    }

};

// ──────────────────────────────────────────────────────────────────────────
// BinaryReader
// ──────────────────────────────────────────────────────────────────────────

/**
 * Read-cursor over an immutable span of bytes.
 *
 * Designed for deserialisation on the client side; does not own memory.
 * All reads advance an internal cursor.  Out-of-bounds reads assert in
 * debug and invoke undefined behaviour in release (callers must validate
 * message length before feeding it to a BinaryReader).
 */
class BinaryReader {
    const std::byte* m_ptr;
    std::size_t      m_remaining;

public:
    explicit BinaryReader(std::span<const std::byte> data) noexcept
        : m_ptr(data.data()), m_remaining(data.size()) {}

    // ── Raw bytes ────────────────────────────────────────────────────────

    void read_bytes(void* dst, std::size_t n) noexcept {
        assert(n <= m_remaining && "BinaryReader underflow");
        std::memcpy(dst, m_ptr, n);
        m_ptr      += n;
        m_remaining -= n;
    }

    std::span<const std::byte> read_bytes(std::size_t n) noexcept {
        assert(n <= m_remaining && "BinaryReader underflow");
        auto span = std::span<const std::byte>{ m_ptr, n };
        m_ptr      += n;
        m_remaining -= n;
        return span;
    }

    // ── Primitive scalar ─────────────────────────────────────────────────

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    T read() noexcept {
        T value;
        read_bytes(&value, sizeof(T));
        return value;
    }

    // ── Codec-dispatched read ─────────────────────────────────────────────

    template<Decodable T>
    T decode() {
        return Codec<T>::decode(*this);
    }

    // ── State ─────────────────────────────────────────────────────────────

    std::size_t remaining() const noexcept { return m_remaining; }
    bool        empty()     const noexcept { return m_remaining == 0; }
};

// ──────────────────────────────────────────────────────────────────────────
// Built-in Codec specialisations for C++ primitives
// ──────────────────────────────────────────────────────────────────────────

// All fixed-width integer and float types that are trivially copyable get
// a direct memcpy codec — no varint encoding, deliberately, because we are
// optimising for throughput not wire-size (and positions are floats anyway).

#define TW_SERIAL_TRIVIAL_CODEC(T)                                      \
    template<>                                                           \
    struct Codec<T> {                                                     \
        static void encode(BinaryWriter& w, const T& v) noexcept {      \
            w.write(v);                                                  \
        }                                                                 \
        static T decode(BinaryReader& r) noexcept {                      \
            return r.read<T>();                                          \
        }                                                                 \
    }

TW_SERIAL_TRIVIAL_CODEC(bool);
TW_SERIAL_TRIVIAL_CODEC(uint8_t);
TW_SERIAL_TRIVIAL_CODEC(uint16_t);
TW_SERIAL_TRIVIAL_CODEC(uint32_t);
TW_SERIAL_TRIVIAL_CODEC(uint64_t);
TW_SERIAL_TRIVIAL_CODEC(int8_t);
TW_SERIAL_TRIVIAL_CODEC(int16_t);
TW_SERIAL_TRIVIAL_CODEC(int32_t);
TW_SERIAL_TRIVIAL_CODEC(int64_t);
TW_SERIAL_TRIVIAL_CODEC(float);
TW_SERIAL_TRIVIAL_CODEC(double);

#undef TW_SERIAL_TRIVIAL_CODEC

} // namespace tw::serial
