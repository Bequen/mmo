#pragma once

#include <cstdint>
namespace tw::net::quicr {

enum FrameType : uint8_t {
    Padding = 0x00,
    KeepAlive = 0x01,
    Ack = 0x02,
    AckEcn = 0x03,
    ResetStream = 0x04,
    StopSending = 0x05,
    Crypto = 0x06,
    NewToken = 0x07,

    // STREAM is 0x08..0x0f (low 3 bits are flags)
    StreamBase = 0x08, // interpret specially

    Hello = 0x10,
    HelloAck = 0x11,
    HandshakeDone = 0x12
};

}
