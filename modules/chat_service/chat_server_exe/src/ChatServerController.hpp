#pragma once

#include "ChatService.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <unordered_map>
#include <vector>

namespace tw::chat {

class ChatServerController {
    static constexpr size_t MAX_TYPES = 32;

    net::quicr::QuicrEndpoint           m_endpoint;
    net::quicr::QuicrConnectionListener m_listener;
    std::unordered_map<uint64_t, net::quicr::QuicrConnection*> m_connections;
    std::vector<std::byte>              m_recv_buf{64 * 1024};
    ChatService                         m_service;

    std::array<std::function<void(uint64_t, std::span<const std::byte>)>, MAX_TYPES> m_handlers{};

public:
    explicit ChatServerController(int port = CHAT_DEFAULT_PORT);

    void update();

private:
    void register_handlers();
    void dispatch(uint64_t client_id, std::span<const std::byte> data);
    void send_to(uint64_t client_id, uint32_t type, std::span<const std::byte> payload,
                 bool reliable = false);
    void broadcast(uint64_t client_id, const ChatMessage& msg);
};

} // namespace tw::chat
