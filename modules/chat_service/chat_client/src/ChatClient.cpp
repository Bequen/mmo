#include "ChatClient.hpp"
#include "Chat.pb.h"
#include "MessageRegistry.hpp"
#include "SendChatMessage.hpp"

namespace {

tl::expected<void, tw::chat::ChatClientError> from_error_code(mmo::chat::ChatErrorCode code) {
    switch (code) {
        case mmo::chat::CHAT_ERROR_CODE_OK:
            return {};
        case mmo::chat::CHAT_ERROR_CODE_CHANNEL_NOT_FOUND:
        case mmo::chat::CHAT_ERROR_CODE_ALREADY_IN_CHANNEL:
        case mmo::chat::CHAT_ERROR_CODE_NOT_IN_CHANNEL:
            return tl::make_unexpected(tw::chat::ChatClientError::ChannelNotFound);
        default:
            return tl::make_unexpected(tw::chat::ChatClientError::PermissionDenied);
    }
}

} // namespace

namespace tw::chat {

ChatClient::ChatClient(const std::string& server_address, int16_t port)
    : m_session(net::Address(server_address, port))
{
    m_session.set_handler(CHAT_MESSAGE_BROADCAST_REQUEST, [this](std::span<const std::byte> data) {
        if (!m_on_message) return;
        mmo::chat::ChatMessageBroadcastRequest proto;
        if (!proto.ParseFromArray(data.data(), static_cast<int>(data.size()))) return;
        ChatMessage msg;
        msg.channel_id = proto.channel_id();
        msg.client_id  = proto.sender_id();
        msg.message    = proto.message();
        msg.timestamp  = ChatMessage::Clock::now();
        m_on_message(std::move(msg));
    });

    m_session.set_handler(CHAT_SEND_MESSAGE_RESPONSE, [this](std::span<const std::byte> data) {
        if (!m_on_send_response) return;
        mmo::chat::SendChatMessageResponse proto;
        if (!proto.ParseFromArray(data.data(), static_cast<int>(data.size()))) return;
        m_on_send_response(from_error_code(proto.error()));
    });

    m_session.set_handler(CHAT_JOIN_CHANNEL_RESPONSE, [this](std::span<const std::byte> data) {
        if (!m_on_join_response) return;
        mmo::chat::JoinChannelResponse proto;
        if (!proto.ParseFromArray(data.data(), static_cast<int>(data.size()))) return;
        m_on_join_response(static_cast<uint32_t>(proto.channel_id()), from_error_code(proto.error()));
    });

    m_session.set_handler(CHAT_LEAVE_CHANNEL_RESPONSE, [this](std::span<const std::byte> data) {
        if (!m_on_leave_response) return;
        mmo::chat::LeaveChannelResponse proto;
        if (!proto.ParseFromArray(data.data(), static_cast<int>(data.size()))) return;
        m_on_leave_response(static_cast<uint32_t>(proto.channel_id()), from_error_code(proto.error()));
    });
}

void ChatClient::update() {
    m_session.update();
}

tl::expected<void, ChatClientError> ChatClient::send_mesg(SendChatMessage message) {
    mmo::chat::SendChatMessageRequest mesg;
    mesg.set_channel_id(message.channel_id);
    mesg.set_message(message.message);

    std::vector<std::byte> buf(mesg.ByteSizeLong());
    (void)mesg.SerializeToArray(buf.data(), static_cast<int>(buf.size()));

    auto send_r = m_session.send(Message<mmo::chat::SendChatMessageRequest>::value,
                                 std::span(buf), true);
    if (!send_r)
        return tl::make_unexpected(ChatClientError::PermissionDenied);
    return {};
}

void ChatClient::set_on_message(std::function<void(ChatMessage)> handler) {
    m_on_message = std::move(handler);
}

void ChatClient::set_on_send_response(std::function<void(tl::expected<void, ChatClientError>)> handler) {
    m_on_send_response = std::move(handler);
}

void ChatClient::set_on_join_response(std::function<void(uint32_t, tl::expected<void, ChatClientError>)> handler) {
    m_on_join_response = std::move(handler);
}

void ChatClient::set_on_leave_response(std::function<void(uint32_t, tl::expected<void, ChatClientError>)> handler) {
    m_on_leave_response = std::move(handler);
}

} // namespace tw::chat
