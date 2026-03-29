#pragma once

#include "common.hpp"
#include <entt/entt.hpp>

namespace tw::net::im {

class InterestResult {
    std::vector<entt::entity> m_entities;
    std::vector<entt::entity> m_despawn;
    std::vector<entt::entity> m_spawn;

public:
    GET_REF(m_entities, entities);
    GET_REF(m_despawn, despawns);
    GET_REF(m_spawn, spawns);

    void set_update(std::vector<entt::entity> updates) {
        m_entities = updates;
    }

    void set_despawn(std::vector<entt::entity> despawn) {
        m_despawn = despawn;
    }

    void set_spawn(std::vector<entt::entity> spawn) {
        m_spawn = spawn;
    }
};

}
