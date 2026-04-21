#pragma once

#include "services/ChatChannelManager.hpp"
#include "services/ChatChannelSubscriptionManager.hpp"
#include "services/ChatMessagePropagator.hpp"

#include <tl/expected.hpp>
#include <cstdint>
#include <functional>
#include <string>

namespace tw::chat {

const int16_t CHAT_DEFAULT_PORT = 8099;

class ChatService {
    ChatChannelSubscriptionManager m_subscriptions;
    ChatMessagePropagator          m_propagator;
    ChatChannelManager             m_channel_manager;

public:
    explicit ChatService(std::function<void(uint64_t, const ChatMessage&)> broadcast_fn);

    void add_channel(ChannelId id, ChatChannel channel);

    tl::expected<void, ChatServerError> send_message(uint64_t client_id,
                                                     uint64_t channel_id,
                                                     const std::string& message);
    void join_channel(uint64_t client_id, uint64_t channel_id);
    void leave_channel(uint64_t client_id, uint64_t channel_id);
};

} // namespace tw::chat
