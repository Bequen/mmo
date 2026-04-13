#pragma once

#include "ClientState.hpp"
#include "interest_management/SpatialBackend.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "world/World.hpp"
#include "world/Transform.hpp"

#include <algorithm>
#include <entt/entt.hpp>
#include <tracy/Tracy.hpp>
#include <unordered_map>

namespace tw::net::im {

/**
 * High-performance interest management system for MMO servers.
 *
 * Maintains the set of "relevant" entities for each connected player based on
 * spatial locality. Uses a templated spatial backend for maximum flexibility:
 * - FixedGrid: bounded worlds, zero allocations, maximum speed
 * - SpatialHashGrid: unbounded worlds, flexible, ~15% slower
 *
 * Design:
 *   Phase 1 — Spatial rebuild (O(E) where E = total entities):
 *     - Rebuild spatial structure from scratch each frame
 *     - Single pass over all Transform components
 *     - O(E) is acceptable at 20 Hz with tens of thousands of entities
 *
 *   Phase 2 — Per-player queries (O(P × K) where P = players, K = neighborhood size):
 *     - For each player: query spatial backend for neighborhood entities
 *     - Sort candidates, compute delta (spawn/despawn) with prev frame
 *     - Independently parallelizable per-player (no shared state mutations)
 *
 * Memory:
 *   - All per-player vectors reused every frame (capacity preserved)
 *   - No heap allocations in steady state after initial setup
 *   - ClientState is move-only to prevent accidental copies
 *
 * Thread-safety:
 *   - Currently single-threaded; Phase 2 per-client updates can be parallelized
 *     with std::execution::par_unseq in future (no shared state)
 *
 * Template parameter:
 *   Backend: must satisfy the SpatialBackend concept
 */
template<SpatialBackend Backend>
class InterestSystem {
    const World*                                    m_world;
    const PlayerSessionRegistry*                    m_session_registry;
    Backend                                         m_backend;
    std::unordered_map<uint32_t, ClientState>      m_client_entities;

public:
    // Constructor: forwards all backend args via perfect forwarding
    template<typename... BackendArgs>
    explicit InterestSystem(
        const World*                    world,
        const PlayerSessionRegistry*    registry,
        BackendArgs&&...                backend_args
    )
        : m_world(world),
          m_session_registry(registry),
          m_backend(std::forward<BackendArgs>(backend_args)...)
    {
    }

    // Registers a client to track interest for the given player entity.
    // Called once per client connection.
    void set_interest_delegate(uint32_t session_id, entt::entity entity) {
        m_client_entities.emplace(session_id, ClientState(entity));
    }

    /**
     * Updates interest sets for all connected clients.
     *
     * Two-phase algorithm:
     *   Phase 1: Rebuild spatial backend (O(E))
     *   Phase 2: Query and compute deltas (O(P × K))
     *
     * Must be called once per frame after PlayerMove messages are processed
     * and before StateReplicator needs the results.
     */
    void update() {
        ZoneScopedN("InterestSystem::update");

        // ─────────────────────────────────────────────────────────────────────
        // Phase 1: Rebuild spatial backend
        // ─────────────────────────────────────────────────────────────────────
        {
            ZoneScopedN("InterestSystem::Phase1_GridRebuild");

            m_backend.begin_frame();

            // Insert all entities with Transform components into the grid
            m_world->registry()
                .view<Transform>()
                .each([this](entt::entity entity, const Transform& transform) {
                    m_backend.insert(entity, transform.position());
                });
        }

        // ─────────────────────────────────────────────────────────────────────
        // Phase 2: Per-client interest queries and delta computation
        // ─────────────────────────────────────────────────────────────────────
        {
            ZoneScopedN("InterestSystem::Phase2_PerClientDeltas");

            for (auto& [client_id, state] : m_client_entities) {
                // Skip clients without a valid entity
                entt::entity player_entity = state.entity();
                if (player_entity == entt::null) {
                    state.spawn().clear();
                    state.despawn().clear();
                    continue;
                }

                // Get the player's current position
                const Transform* player_transform =
                    m_world->registry().try_get<Transform>(player_entity);
                if (!player_transform) {
                    // Player has no transform; clear interest
                    state.spawn().clear();
                    state.despawn().clear();
                    // Swap with empty vector to clear interest
                    std::vector<entt::entity> empty;
                    state.swap_interest(empty);
                    continue;
                }

                // Query spatial backend for neighborhood entities
                // m_new_interest is cleared before use
                state.new_interest().clear();
                m_backend.query_neighbors(
                    player_transform->position(),
                    state.new_interest()
                );

                // Sort candidates for set_difference comparison
                std::sort(
                    state.new_interest().begin(),
                    state.new_interest().end()
                );

                // Compute spawn set: entities in new but not in old
                state.spawn().clear();
                std::set_difference(
                    state.new_interest().begin(),
                    state.new_interest().end(),
                    state.interest().begin(),
                    state.interest().end(),
                    std::back_inserter(state.spawn())
                );

                // Compute despawn set: entities in old but not in new
                state.despawn().clear();
                std::set_difference(
                    state.interest().begin(),
                    state.interest().end(),
                    state.new_interest().begin(),
                    state.new_interest().end(),
                    std::back_inserter(state.despawn())
                );

                // Finalize: swap new into persistent interest set
                state.swap_interest(state.new_interest());
            }
        }
    }

    /**
     * Returns a pointer to the ClientState for the given client.
     *
     * Returns nullptr if the client is not registered.
     * The pointer remains valid until the next call to update() or
     * set_interest_delegate(), or until the client is unregistered.
     *
     * Zero-copy: callers can access interest, spawn, and despawn vectors
     * without incurring any allocations.
     */
    const ClientState* get_player_interest(uint32_t session_id) const {
        auto it = m_client_entities.find(session_id);
        if (it == m_client_entities.end()) {
            return nullptr;
        }
        return &it->second;
    }
};

} // namespace tw::net::im
