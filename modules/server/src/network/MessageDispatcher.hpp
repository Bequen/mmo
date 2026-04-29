#pragma once

#include "InboundMessage.hpp"
#include "MessageQueue.hpp"
#include "NetworkError.hpp"
#include "MessageDeserializer.hpp"
#include "monitoring/TimescaleDbMetricsReporter.hpp"
#include "network/NetworkReceiver.hpp"
#include "packets/Packet.hpp"

#include <tl/expected.hpp>

#include <functional>
#include <tracy/Tracy.hpp>

namespace tw::net {

typedef std::function<tl::expected<void, NetworkError>(uint32_t, std::span<std::byte>)> MessageHandler;

class MessageDispatcher {
    NetworkReceiver* m_network;

    std::unordered_map<uint32_t, MessageHandler>        m_handlers;

public:
    MessageDispatcher(NetworkReceiver* network) :
        m_network(network) {
    }

    template<typename T>
    void set_handler(std::function<void(uint32_t, T)> handler) {
        m_handlers[static_cast<uint32_t>(Message<T>::value)] =
            [handler, this](uint32_t session_id, std::span<std::byte> data) -> tl::expected<void, NetworkError> {
                ZoneScopedN("Handling message");

                size_t size = 0;
                auto r = MessageDeserializer::deserialize<T>(data);
                if(!r) {
                    return {};
                }

                handler(session_id, *r);
                return {};
            };
    }

    void drain_queue() {
        while(!m_network->inbound_queue()->is_empty()) {
            auto msg = m_network->inbound_queue()->pop();
            uint32_t message_type = *(uint32_t*)msg->payload.data();

            if(m_handlers.find(message_type) == m_handlers.end()) {
                spdlog::error("No handler for message type {}", message_type);
                continue;
            }

            auto r = m_handlers[message_type](msg->session_id, std::span<std::byte>(msg->payload).subspan(sizeof(uint32_t)));
            if(!r) {
                spdlog::error("Failed to handle message of type {}: {}", message_type, r.error().message());
                // TODO: Add session failure
                continue;
            }
        }
    }
};

}
