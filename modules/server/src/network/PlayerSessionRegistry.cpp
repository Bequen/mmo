#include "PlayerSessionRegistry.hpp"
#include "PlayerSession.hpp"
#include "protocol/quicr/QuicrConnection.hpp"

namespace tw::net {

SessionId PlayerSessionRegistry::generate_session_id() {
    static SessionId next_id = 1;
    return next_id++;
}

SessionId PlayerSessionRegistry::register_session(quicr::QuicrConnection* quicr_connection) {
    spdlog::info("Registering new session");

    auto session = new PlayerSession(quicr_connection->self_id(), quicr_connection);
    m_session_vec.emplace_back(session);

    m_session_map.emplace(quicr_connection->self_id(), session);

    return quicr_connection->self_id();
}

}
