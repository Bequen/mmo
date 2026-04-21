#pragma once

#include "models/ChatMessage.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace tw::chat {

class ChatChannel {
    uint64_t id;
    std::string name;

    std::vector<ChatMessage> messages;

public:
    ChatChannel(uint64_t id, std::string name) : id(id), name(std::move(name)) {}

    bool add_message(const ChatMessage& message) {
        messages.push_back(message);
        return true;
    }
};

}
