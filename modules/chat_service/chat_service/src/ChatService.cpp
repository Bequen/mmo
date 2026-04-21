#include "ChatService.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace tw::chat {

ChatService::ChatService(std::function<void(uint64_t, const ChatMessage&)> broadcast_fn)
    : m_propagator(&m_subscriptions, std::move(broadcast_fn))
    , m_channel_manager(&m_propagator)
{
    m_channel_manager.add_channel(1, ChatChannel(1, "Test"));
}

void ChatService::add_channel(ChannelId id, ChatChannel channel) {
    m_channel_manager.add_channel(id, std::move(channel));
}

tl::expected<void, ChatServerError> ChatService::send_message(
    uint64_t client_id, uint64_t channel_id, const std::string& message)
{
    const auto& subscribers = m_subscriptions.get_subscribers(channel_id);
    const bool is_subscribed = std::ranges::find(subscribers, client_id) != subscribers.end();
    if (!is_subscribed)
        return tl::unexpected(ChatServerError::PermissionDenied);
    return m_channel_manager.add_message(client_id, channel_id, message);
}

void ChatService::join_channel(uint64_t client_id, uint64_t channel_id) {
    m_subscriptions.add_subscription(client_id, channel_id);
}

void ChatService::leave_channel(uint64_t client_id, uint64_t channel_id) {
    m_subscriptions.remove_subscription(client_id, channel_id);
}

} // namespace tw::chat
