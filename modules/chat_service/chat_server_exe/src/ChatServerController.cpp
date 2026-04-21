#include "ChatServerController.hpp"

#include "Chat.pb.h"
#include "MessageRegistry.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace tw::chat {

static mmo::chat::ChatErrorCode to_error_code(tl::expected<void, ChatServerError> result) {
    if (result) return mmo::chat::CHAT_ERROR_CODE_OK;
    switch (result.error()) {
        case ChatServerError::PermissionDenied: return mmo::chat::CHAT_ERROR_CODE_NOT_IN_CHANNEL;
        case ChatServerError::ChannelNotFound:  return mmo::chat::CHAT_ERROR_CODE_CHANNEL_NOT_FOUND;
    }
    return mmo::chat::CHAT_ERROR_CODE_CHANNEL_NOT_FOUND;
}

template<typename T>
static std::vector<std::byte> serialize(const T& msg) {
    std::vector<std::byte> buf(msg.ByteSizeLong());
    (void)msg.SerializeToArray(buf.data(), static_cast<int>(buf.size()));
    return buf;
}

ChatServerController::ChatServerController(int port)
    : m_endpoint(net::quicr::QuicrEndpoint::create_and_bind(port).value())
    , m_listener(net::quicr::QuicrConnectionListener::listen(&m_endpoint).value())
    , m_service([this](uint64_t id, const ChatMessage& msg) { broadcast(id, msg); })
{
    m_endpoint.assign_listener(&m_listener);
    register_handlers();
    spdlog::info("Chat server listening on port {}", port);
}

void ChatServerController::register_handlers() {
    m_handlers[Message<mmo::chat::SendChatMessageRequest>::value] =
        [this](uint64_t client_id, std::span<const std::byte> data) {
            mmo::chat::SendChatMessageRequest msg;
            msg.ParseFromArray(data.data(), static_cast<int>(data.size()));
            mmo::chat::SendChatMessageResponse r;
            r.set_channel_id(msg.channel_id());
            r.set_error(to_error_code(m_service.send_message(client_id, msg.channel_id(), msg.message())));
            send_to(client_id, Message<mmo::chat::SendChatMessageResponse>::value, serialize(r));
        };

    m_handlers[Message<mmo::chat::JoinChannelRequest>::value] =
        [this](uint64_t client_id, std::span<const std::byte> data) {
            mmo::chat::JoinChannelRequest msg;
            msg.ParseFromArray(data.data(), static_cast<int>(data.size()));
            m_service.join_channel(client_id, msg.channel_id());
            mmo::chat::JoinChannelResponse r;
            r.set_channel_id(msg.channel_id());
            r.set_error(mmo::chat::CHAT_ERROR_CODE_OK);
            send_to(client_id, Message<mmo::chat::JoinChannelResponse>::value, serialize(r));
        };

    m_handlers[Message<mmo::chat::LeaveChannelRequest>::value] =
        [this](uint64_t client_id, std::span<const std::byte> data) {
            mmo::chat::LeaveChannelRequest msg;
            msg.ParseFromArray(data.data(), static_cast<int>(data.size()));
            m_service.leave_channel(client_id, msg.channel_id());
            mmo::chat::LeaveChannelResponse r;
            r.set_channel_id(msg.channel_id());
            r.set_error(mmo::chat::CHAT_ERROR_CODE_OK);
            send_to(client_id, Message<mmo::chat::LeaveChannelResponse>::value, serialize(r));
        };
}

void ChatServerController::update() {
    m_endpoint.poll();

    net::quicr::QuicrConnection* conn = nullptr;
    while ((conn = m_listener.listen())) {
        m_connections.emplace(conn->self_id(), conn);
        spdlog::info("Chat client connected: {}", conn->self_id());
    }

    for (auto& [client_id, conn] : m_connections) {
        auto r = conn->read_into(m_recv_buf);
        if (!r || *r == 0) continue;
        dispatch(client_id, std::span(m_recv_buf.data(), *r));
    }
}

void ChatServerController::dispatch(uint64_t client_id, std::span<const std::byte> data) {
    constexpr size_t HEADER = sizeof(uint32_t) * 2;
    if (data.size() < HEADER) {
        spdlog::warn("ChatServerController: dropped short datagram ({} bytes)", data.size());
        return;
    }
    uint32_t type{};
    std::memcpy(&type, data.data(), sizeof(type));

    if (type >= m_handlers.size() || !m_handlers[type]) {
        spdlog::warn("ChatServerController: no handler for type {}", type);
        return;
    }
    m_handlers[type](client_id, data.subspan(HEADER));
}

void ChatServerController::send_to(uint64_t client_id, uint32_t type,
                                    std::span<const std::byte> payload, bool reliable) {
    auto it = m_connections.find(client_id);
    if (it == m_connections.end()) return;

    constexpr uint32_t SEQ_NONE = 0;
    std::vector<std::byte> buf(sizeof(type) + sizeof(SEQ_NONE) + payload.size());
    std::memcpy(buf.data(),                    &type,     sizeof(type));
    std::memcpy(buf.data() + sizeof(type),     &SEQ_NONE, sizeof(SEQ_NONE));
    std::memcpy(buf.data() + sizeof(type) + sizeof(SEQ_NONE), payload.data(), payload.size());
    (void)it->second->send_message(std::span(buf), reliable);
}

void ChatServerController::broadcast(uint64_t client_id, const ChatMessage& msg) {
    mmo::chat::ChatMessageBroadcastRequest bcast;
    bcast.set_channel_id(msg.channel_id);
    bcast.set_sender_id(msg.client_id);
    bcast.set_message(msg.message);
    send_to(client_id, Message<mmo::chat::ChatMessageBroadcastRequest>::value,
            serialize(bcast), true);
}

} // namespace tw::chat
