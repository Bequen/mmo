#pragma once

#include "ByteBuffer.hpp"
#include "bytebuffer/ByteBufferCodec.hpp"

namespace tw::net {


struct ByteBufferDecoder {
    ByteBufferDecoder(RingByteBuffer& buf) : m_buf(buf) {}

    template<typename T>
    std::optional<T> push(size_t offset = 0)
    {
        std::optional<T> s = ByteBufferCodec<T>::bytes(m_buf, offset);
        m_buf.skip(sizeof(T));

        return s;
    }

private:
    RingByteBuffer& m_buf;
};

}
