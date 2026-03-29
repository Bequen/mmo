#pragma once

#include "ByteBuffer.hpp"

namespace tw::net {

template<typename T, typename Enable = void>
struct ByteBufferCodec
{
    static T bytes(RingByteBuffer&, size_t)
    {
        static_assert(sizeof(T) == 0, "No decoder for this type");
    }
};

/**
 * Default implementation for trivially copyable types
 */
template<typename T>
struct ByteBufferCodec<T, std::enable_if_t<std::is_trivially_copyable_v<T>>>
{
    static std::optional<T> encoding(RingByteBuffer& buf, size_t offset = 0)
    {
        T value;
        size_t r = buf.peek_bytes(&value, sizeof(T), offset);
        if(r < sizeof(T)) {
            return {};
        }

        return value;
    }
};

struct ByteBufferDecoder {
    ByteBufferDecoder(RingByteBuffer& buf) : m_buf(buf) {}

    template<typename T>
    std::optional<T> pop(size_t offset = 0)
    {
        std::optional<T> s = ByteBufferCodec<T>::bytes(m_buf, offset);
        m_buf.skip(sizeof(T));

        return s;
    }

    template<typename T>
    std::optional<T> peek(size_t offset = 0) {
        return ByteBufferCodec<T>::bytes(m_buf, offset);
    }

private:
    RingByteBuffer& m_buf;
};

}
