#pragma once

#include "common.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"

#include <cstddef>
#include <vector>

namespace tw::net {

class Frame {
    quicr::FrameType m_frame_type;
    std::vector<std::byte> m_buffer;

public:
    GET(m_frame_type, frame_type);
    GET_REF(m_buffer, buffer);

    Frame(quicr::FrameType type, std::vector<std::byte> buffer) :
        m_frame_type(type),
        m_buffer(buffer) {};
};

}
