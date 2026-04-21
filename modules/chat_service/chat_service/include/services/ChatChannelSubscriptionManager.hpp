#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace tw::chat {

class ChatChannelSubscriptionManager {
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_client_channels;
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_channel_clients;

public:
    void add_subscription(uint64_t client_id, uint64_t channel_id);
    void remove_subscription(uint64_t client_id, uint64_t channel_id);
    void get_subscriptions(uint64_t client_id, std::vector<uint64_t>& out) const;
    const std::vector<uint64_t>& get_subscribers(uint64_t channel_id) const;
};

} // namespace tw::chat
