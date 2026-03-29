#pragma once

#include "bytebuffer/ByteBuffer.hpp"
#include <type_traits>

namespace tw::net {
template<typename T, typename Enable = void>
struct ByteBufferCodec
{
    static size_t encoding(RingByteBuffer&, T*, size_t offset)
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
    static size_t encoding(RingByteBuffer& buf, T* target, size_t offset)
    {
        return buf.peek_bytes(target, sizeof(T), offset);
    }
};
}
