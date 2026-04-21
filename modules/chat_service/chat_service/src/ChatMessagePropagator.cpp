#include "services/ChatMessagePropagator.hpp"

namespace tw::chat {

ChatMessagePropagator::ChatMessagePropagator(
    const ChatChannelSubscriptionManager* subscriptions,
    std::function<void(uint64_t, const ChatMessage&)> send
) :
    m_subscriptions(subscriptions),
    m_send(std::move(send))
{}

void ChatMessagePropagator::on_message_sent(const ChatMessage& message) {
    for (uint64_t client_id : m_subscriptions->get_subscribers(message.channel_id))
        m_send(client_id, message);
}

} // namespace tw::chat
