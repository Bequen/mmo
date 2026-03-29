#pragma once

#include "TcpListener.hpp"
#include "frames/Frame.hpp"
#include "MessageQueue.hpp"
#include "InboundMessage.hpp"
#include "packets/LoginPacket.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"

namespace tw::net {

class NetworkReceiver {
    PlayerSessionRegistry *m_session_registry;

    TcpListener m_tcp_listener;
    std::unique_ptr<quicr::QuicrEndpoint> m_quicr_endpoint;
    std::unique_ptr<quicr::QuicrConnectionListener> m_quicr_listener;

    MessageQueue<InboundMessage*>* m_inbound_queue;

    std::deque<SessionId> m_new_sessions;

    bool listen_tcp();

    bool listen_quicr();

    void listen();

    void process_frame(uint32_t session_id, const Frame& frame);

    void process_streams();

public:
    NetworkReceiver(
        PlayerSessionRegistry* session_registry,
        int32_t tcp_port,
        int32_t udp_port
    );

    MessageQueue<InboundMessage*>* inbound_queue() {
        return m_inbound_queue;
    }

    bool peek_new_session() const {
        return !m_new_sessions.empty();
    }

    SessionId pop_new_session() {
        auto session = m_new_sessions.front();
        m_new_sessions.pop_front();
        return session;
    }

    void update();

    template<typename T>
    size_t send_mesg(SessionId session_id, const T& mesg) {
        std::string payload;
        if(!mesg.SerializeToString(&payload)) {
            spdlog::error("Failed to serialize message");
            return 0;
        }

        int32_t length = payload.length();
        if(length == 0) {
            return 0;
        }

        std::vector<std::byte> bytes(length + sizeof(uint32_t));

        uint32_t type = Message<T>::value;
        auto payload_bytes = std::as_writable_bytes(std::span(payload));
        memcpy(bytes.data(), &type, sizeof(type));
        memcpy(bytes.data() + sizeof(uint32_t), payload_bytes.data(), payload_bytes.size());

        auto session = m_session_registry->session(session_id);

        session->quicr_connection->push_stream_frame(bytes, true);
        return bytes.size();
    }
};

}
