#pragma once

#include "bytebuffer/ByteBuffer.hpp"
#include "frames/Frame.hpp"

namespace tw::net::frame {

class FrameCodec {
public:
    static void encode(RingByteBuffer& target, const Frame& frame);

    static std::vector<Frame> decode(RingByteBuffer& target);
};

}
