#pragma once

#include "MessageSession.hpp"
#include "SendChatMessage.hpp"
#include "ChatClientError.hpp"
#include "models/ChatMessage.hpp"
#include <functional>
#include <string>

namespace tw::chat {

class ChatClient {
    tw::MessageSession m_session;

    std::function<void(ChatMessage)>                                      m_on_message;
    std::function<void(tl::expected<void, ChatClientError>)>              m_on_send_response;
    std::function<void(uint32_t, tl::expected<void, ChatClientError>)>    m_on_join_response;
    std::function<void(uint32_t, tl::expected<void, ChatClientError>)>    m_on_leave_response;

public:
    ChatClient(const std::string& server_address, int16_t port);

    void update();

    tl::expected<void, ChatClientError> send_mesg(SendChatMessage message);

    // Called for every broadcast message received on a subscribed channel.
    void set_on_message(std::function<void(ChatMessage)> handler);

    // Called when the server responds to send/join/leave requests.
    // tl::expected holds void on success, ChatClientError on failure.
    void set_on_send_response(std::function<void(tl::expected<void, ChatClientError>)> handler);
    void set_on_join_response(std::function<void(uint32_t channel_id, tl::expected<void, ChatClientError>)> handler);
    void set_on_leave_response(std::function<void(uint32_t channel_id, tl::expected<void, ChatClientError>)> handler);
};

}
