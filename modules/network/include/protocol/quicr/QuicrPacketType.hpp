#pragma once

#include <cstdint>

namespace tw::net::quicr {

enum class QuicrPacketType : uint8_t {
    Unknown,
    Initial,
    Handshake,
    Established
};

}
