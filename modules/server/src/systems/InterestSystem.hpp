#pragma once

#include "ClientManager.hpp"
#include "PlayerClient.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "systems/InterestResult.hpp"
#include "world/World.hpp"

#include <entt/entt.hpp>

namespace tw::net::im {

class ClientState {
    entt::entity m_entity;
    std::set<entt::entity> m_interest;
    std::vector<entt::entity> m_despawn;
    std::vector<entt::entity> m_spawn;

public:
    GET(m_entity, entity);
    GET_MUT_REF(m_interest, interest);

    GET_MUT_REF(m_despawn, despawn);
    GET_MUT_REF(m_spawn, spawn);

    ClientState(entt::entity entity) : m_entity(entity) {}
};

/**
 * System for each property handling what is it subscribing to.
 * Based on the value is determine which topics are of interest. The topics
 * can be cells of players in spatial grids etc.
 */
class InterestSystem {
    const World* m_world;

    const PlayerSessionRegistry* m_session_registry;

    std::unordered_map<PlayerClientId, ClientState> m_client_entities;

    const ClientState* get_client_state(PlayerClientId player_client_id) const {
        auto it = m_client_entities.find(player_client_id);
        if(it != m_client_entities.end()) {
            return &it->second;
        }
        return nullptr;
    }

public:
    InterestSystem(const World* world, const PlayerSessionRegistry *registry);

    void set_interest_delegate(uint32_t session_id, entt::entity entity) {
        m_client_entities.emplace(session_id, entity);
    }

    /**
     * Updates players interest state based on the changes.
     */
    void update();

    /**
     * Returns player's interest.
     */
    const InterestResult get_player_interest(PlayerClientId player_client_id) const;
};
}
