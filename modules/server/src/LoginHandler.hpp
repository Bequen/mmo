#pragma once

#include <vector>

#include "Messenger.hpp"
#include "PlayerClient.hpp"
#include "packets/LoginPacket.hpp"

namespace tw::net {

enum LoginState {
    GUEST,
    LOGGED_IN,
    REFUSED
};

struct LoginClient {
    LoginState state;
    Messenger messenger;
};

struct LoggedInClient {
    std::string name;
    Messenger messenger;
};

class LoginHandler {
private:
    std::vector<LoginClient> m_queue;

    std::vector<LoggedInClient> m_logged_in;

    bool can_log_in(LoginPacket& packet);

    bool handle_login_packet(Messenger& messenger, LoginPacket&& packet) {
        if(can_log_in(packet)) {
            m_logged_in.push_back({
                    std::string(packet.username, packet.username_length), 
                    std::move(messenger)
            });

            std::println("User {} logged in", m_logged_in.back().name);

            return true;
        }

        return false;
    }

public:
    std::vector<LoggedInClient>& get_successfull_logins() {
        return m_logged_in;
    }

    void push(Messenger&& socket);

    void update();
};

}
