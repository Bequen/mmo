#include "services/ChatChannelManager.hpp"

namespace tw::chat {

ChatChannelManager::ChatChannelManager(ChatMessagePropagator* propagator)
    : m_propagator(propagator) {}

bool ChatChannelManager::has_channel(ChannelId channel_id) const {
    return m_channels.contains(channel_id);
}

ChatChannel& ChatChannelManager::get_channel(ChannelId channel_id) {
    return m_channels.at(channel_id);
}

void ChatChannelManager::add_channel(ChannelId channel_id, ChatChannel channel) {
    m_channels.emplace(channel_id, std::move(channel));
}

void ChatChannelManager::remove_channel(ChannelId channel_id) {
    m_channels.erase(channel_id);
}

bool ChatChannelManager::can_client_write(ChannelId channel_id, uint64_t) const {
    return has_channel(channel_id);
}

tl::expected<void, ChatServerError> ChatChannelManager::add_message(uint64_t client_id, uint64_t channel_id, const std::string& message) {
    if (!has_channel(channel_id))
        return tl::unexpected(ChatServerError::ChannelNotFound);
    ChatMessage msg{ChatMessage::Clock::now(), client_id, channel_id, message};
    get_channel(channel_id).add_message(msg);
    m_propagator->on_message_sent(msg);
    return {};
}

} // namespace tw::chat
