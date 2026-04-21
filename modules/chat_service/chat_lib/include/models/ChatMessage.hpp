#pragma once

#include <chrono>

namespace tw::chat {

class ChatMessage {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    TimePoint timestamp;

    uint64_t client_id;
    uint64_t channel_id;
    std::string message;

};

}
