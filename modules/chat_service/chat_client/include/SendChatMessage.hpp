#pragma once

#include <cstdint>
#include <string>

namespace tw::chat {

struct SendChatMessage {
    uint64_t channel_id;
    std::string message;
};

}
