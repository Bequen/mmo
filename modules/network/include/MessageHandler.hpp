#pragma once

#include <functional>

#include "ByteBuffer.hpp"
#include "Messenger.hpp"
#include "messages/PlayerMoveMessage.hpp"
#include "packets/Packet.hpp"

namespace tw::net {

/**
 * Contains handlers for each message type. Calls this handler when message comes in.
 */
class MessageHandler {
private:
    Messenger m_server_messenger;
    std::vector<std::function<void(Messenger&)>> m_handlers;

public:
    MessageHandler(Messenger&& server_messenger) :
        m_server_messenger(std::move(server_messenger)),
        m_handlers(100) {

    }

    template<typename T>
    constexpr void set_handler(const std::function<void(T*)> handler) {
        PacketType type = Message<T>::value;
        m_handlers[type] = [handler](Messenger& messenger) {
            T mesg = messenger.pop<T>();
            handler(&mesg);
        };
    }

    void update() {
        m_server_messenger.fetch();

        while(m_server_messenger.peek().has_value()) {
            PacketType type = m_server_messenger.peek().value();
            if(type >= m_handlers.size() || m_handlers[type] == nullptr) {
                m_server_messenger.clear();
                break;
            }
            m_handlers[type](m_server_messenger);
        }
    }

    template<Serializable T>
    void send(T& mesg) {
        m_server_messenger.send(mesg);
    }

    template<Serializable T>
    void request(T& mesg, std::function<void(PacketType)> handler) {
        
    }
};

}
