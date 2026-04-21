#pragma once

#include "services/ChatMessagePropagator.hpp"
#include "models/ChannelId.hpp"
#include "models/ChatChannel.hpp"
#include "ChatServerError.hpp"

#include <tl/expected.hpp>
#include <string>
#include <unordered_map>

namespace tw::chat {

class ChatChannelManager {
    std::unordered_map<ChannelId, ChatChannel> m_channels;
    ChatMessagePropagator* m_propagator;

public:
    explicit ChatChannelManager(ChatMessagePropagator* propagator);

    bool has_channel(ChannelId channel_id) const;
    ChatChannel& get_channel(ChannelId channel_id);
    void add_channel(ChannelId channel_id, ChatChannel channel);
    void remove_channel(ChannelId channel_id);
    bool can_client_write(ChannelId channel_id, uint64_t client_id) const;
    tl::expected<void, ChatServerError> add_message(uint64_t client_id, uint64_t channel_id, const std::string& message);
};

} // namespace tw::chat
