#pragma once

#include "protocol/quicr/QuicrConnection.hpp"

#include <cstdint>

namespace tw::net {

struct PlayerSession {
public:
    uint32_t session_id;

    quicr::QuicrConnection* quicr_connection;

    uint32_t last_frame;

    PlayerSession(uint32_t session_id, quicr::QuicrConnection* quicr_connection) :
        session_id(session_id),
        quicr_connection(std::move(quicr_connection)),
        last_frame(0)
    { }
};

}
