#pragma once

#include "PlayerSession.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace tw::net {

typedef uint32_t SessionId;

class PlayerSessionRegistry {
    std::vector<PlayerSession*> m_session_vec;

    std::unordered_map<SessionId, std::unique_ptr<PlayerSession>> m_session_map;

    SessionId generate_session_id();

public:
    const std::vector<PlayerSession*>& sessions() const {
        return m_session_vec;
    }

    PlayerSession* session(SessionId id) {
        auto session = m_session_map.find(id);
        return session != m_session_map.end() ? session->second.get() : nullptr;
    }

    SessionId register_session(quicr::QuicrConnection* quicr_connection);

    void unregister_session(SessionId);
};

}
