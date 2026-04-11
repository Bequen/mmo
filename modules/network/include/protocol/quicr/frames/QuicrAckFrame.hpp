#pragma once

#include "protocol/quicr/QuicrEncoder.hpp"

namespace tw::net::quicr {

class QuicrAckFrame {
public:
    QuicrAckFrame() = default;

};

template<>
class QuicrFrameCodec<QuicrAckFrame> {
public:
    static size_t encode(ByteBufferWriter& writer, QuicrAckFrame& frame) {

    }

    static QuicrAckFrame decode(ByteBufferReader& reader) {

    }
};

}
