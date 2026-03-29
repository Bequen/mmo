#pragma once

namespace tw::quicr {
    enum QuicrConnectionState {
        AwaitingHello = 0,
        AwaitingHelloAck = 1,
        Established = 2,
        TimedOut = 3
    };
}
