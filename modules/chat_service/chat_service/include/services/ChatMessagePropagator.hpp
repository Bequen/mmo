#pragma once

#include "services/ChatChannelSubscriptionManager.hpp"
#include "models/ChatMessage.hpp"

#include <cstdint>
#include <functional>

namespace tw::chat {

class ChatMessagePropagator {
    const ChatChannelSubscriptionManager* m_subscriptions;
    std::function<void(uint64_t client_id, const ChatMessage&)> m_send;

public:
    ChatMessagePropagator(
        const ChatChannelSubscriptionManager* subscriptions,
        std::function<void(uint64_t, const ChatMessage&)> send
    );

    void on_message_sent(const ChatMessage& message);
};

} // namespace tw::chat
