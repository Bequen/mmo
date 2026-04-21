#include "services/ChatChannelSubscriptionManager.hpp"

namespace tw::chat {

void ChatChannelSubscriptionManager::add_subscription(uint64_t client_id, uint64_t channel_id) {
    m_client_channels[client_id].push_back(channel_id);
    m_channel_clients[channel_id].push_back(client_id);
}

void ChatChannelSubscriptionManager::remove_subscription(uint64_t client_id, uint64_t channel_id) {
    std::erase(m_client_channels[client_id], channel_id);
    std::erase(m_channel_clients[channel_id], client_id);
}

void ChatChannelSubscriptionManager::get_subscriptions(uint64_t client_id, std::vector<uint64_t>& out) const {
    if (auto it = m_client_channels.find(client_id); it != m_client_channels.end())
        out = it->second;
}

const std::vector<uint64_t>& ChatChannelSubscriptionManager::get_subscribers(uint64_t channel_id) const {
    static const std::vector<uint64_t> empty;
    auto it = m_channel_clients.find(channel_id);
    return it != m_channel_clients.end() ? it->second : empty;
}

} // namespace tw::chat
