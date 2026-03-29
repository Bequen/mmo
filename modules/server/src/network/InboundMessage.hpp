#pragma once

#include <cstdint>
#include <vector>

namespace tw::net {

class InboundMessage {
public:
    uint32_t session_id;
    std::vector<std::byte> payload;

    InboundMessage(uint32_t session_id, std::vector<std::byte> payload)
        : session_id(session_id), payload(std::move(payload)) {}
};

}
