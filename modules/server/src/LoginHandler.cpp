#include "LoginHandler.hpp"
#include "packets/LoginPacket.hpp"

namespace tw::net {

void LoginHandler::push(Messenger&& messenger) {
    m_queue.push_back({GUEST, std::move(messenger)});
}

bool LoginHandler::can_log_in(LoginPacket& packet) {
    return true;
}

void LoginHandler::update() {
    std::erase_if(m_queue, [](const auto& client) {
            return client.messenger.is_closed() || client.state != GUEST;
        });

    for(auto& guest : m_queue) {
        guest.messenger.fetch();

        if(guest.messenger.is_closed()) {
            continue;
        }

        while(guest.messenger.peek().has_value()) {
            switch(guest.messenger.peek().value()) {
                case PacketType::LOGIN_PACKET:
                    if(handle_login_packet(guest.messenger, guest.messenger.pop<LoginPacket>())) {
                        guest.state = LOGGED_IN;
                    } else {
                        guest.state = REFUSED;
                    }
                    break;
                default: break;
            }
        }
    }
}

}
