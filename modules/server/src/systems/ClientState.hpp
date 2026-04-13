#pragma once

#include "common.hpp"
#include <entt/entt.hpp>
#include <vector>

namespace tw::net::im {

/**
 * Tracks the interest state and entity set for a single connected client.
 *
 * The interest set is maintained as a sorted vector for efficient delta computation
 * using std::set_difference. The vector is reused across frames (capacity preserved),
 * so no heap allocation occurs in the steady state.
 *
 * Members:
 *   m_entity: the player's own entity in the world
 *   m_interest: sorted vector of entities the player can see (persistent)
 *   m_new_interest: scratch buffer for candidate entities from spatial query (reused)
 *   m_spawn: entities that entered interest this frame
 *   m_despawn: entities that left interest this frame
 */
class ClientState {
    entt::entity              m_entity;
    std::vector<entt::entity> m_interest;       // sorted, persistent
    std::vector<entt::entity> m_new_interest;   // scratch, reused each frame
    std::vector<entt::entity> m_spawn;
    std::vector<entt::entity> m_despawn;

public:
    GET(m_entity, entity);
    GET_REF(m_interest, interest);
    GET_MUT_REF(m_spawn,   spawn);
    GET_MUT_REF(m_despawn, despawn);

    // Internal accessor for InterestSystem to swap interest sets
    std::vector<entt::entity>& new_interest() {
        return m_new_interest;
    }

    // Swaps m_interest with the provided vector. Used by InterestSystem::update()
    // to atomically finalize the new interest set for this frame.
    void swap_interest(std::vector<entt::entity>& other) {
        m_interest.swap(other);
    }

    bool is_interested_in_entity(entt::entity e) const {
        return std::binary_search(m_interest.begin(), m_interest.end(), e);
    }

    explicit ClientState(entt::entity entity)
        : m_entity(entity)
    {
        // Pre-allocate reasonable capacity to avoid reallocations in steady state.
        // For a typical view radius of 1000 units with cell size 500, this is
        // ~9 cells × ~50 entities per cell = ~450 entities per player.
        m_interest.reserve(512);
        m_new_interest.reserve(512);
        m_spawn.reserve(256);
        m_despawn.reserve(256);
    }
};

} // namespace tw::net::im
