#pragma once

#include "Interest.hpp"
#include "interest_management/SpatialBackend.hpp"
#include "world/Transform.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <entt/entt.hpp>
#include <tracy/Tracy.hpp>
#include <unordered_map>

namespace tw::net::im {

/**
 * Spatial interest management: maintains per-subscription spawn/despawn deltas.
 *
 * Each subscription is identified by a caller-chosen InterestId (uint32_t).
 * Two kinds of subscription:
 *   set_entity_interest(id, entity) — tracks entities in the neighbourhood of
 *                                     a world entity's position (e.g. a player).
 *   set_area_interest(id, bounds)   — tracks entities inside an XZ quad
 *                                     (e.g. a zone border region).
 *
 * The caller decides how ids map to sessions, zones, or anything else.
 * InterestSystem has no knowledge of that mapping.
 *
 * Two-phase update per frame:
 *   Phase 1 — Spatial rebuild (O(E)): insert all Transform entities into the backend.
 *   Phase 2 — Per-subscription deltas (O(N × K)): for each subscription query the
 *             backend, sort results, compute spawn/despawn via set_difference.
 */
template<SpatialBackend Backend>
class InterestSystem {
    const World*                             m_world;
    Backend*                                 m_backend;  // non-owning; caller manages lifetime
    std::unordered_map<InterestId, Interest> m_interests;

public:
    explicit InterestSystem(const World* world, Backend* backend)
        : m_world(world),
          m_backend(backend)
    {}

    // Registers or replaces a subscription tracking the neighbourhood of an entity.
    void set_entity_interest(InterestId id, entt::entity entity) {
        m_interests.insert_or_assign(id, Interest(entity));
    }

    // Registers or replaces a subscription tracking entities inside an XZ quad.
    void set_area_interest(InterestId id, AreaBounds bounds) {
        m_interests.insert_or_assign(id, Interest(bounds));
    }

    // Removes a subscription. No-op if id is not registered.
    void remove_interest(InterestId id) {
        m_interests.erase(id);
    }

    // Returns the current interest state for the given id, or nullptr.
    const Interest* get_interest(InterestId id) const {
        auto it = m_interests.find(id);
        return it != m_interests.end() ? &it->second : nullptr;
    }

    // Updates all subscriptions for the current frame.
    // Must be called once per frame after world positions have been updated.
    void update() {
        ZoneScopedN("InterestSystem::update");

        // ── Phase 1: Rebuild spatial backend ─────────────────────────────────
        {
            ZoneScopedN("InterestSystem::Phase1_GridRebuild");
            m_backend->begin_frame();
            m_world->registry()
                .view<Transform>()
                .each([this](entt::entity e, const Transform& t) {
                    m_backend->insert(e, t.position());
                });
        }

        // ── Phase 2: Per-subscription deltas ─────────────────────────────────
        {
            ZoneScopedN("InterestSystem::Phase2_Deltas");

            for (auto& [id, interest] : m_interests) {
                interest.new_interest().clear();

                if (interest.is_entity_interest()) {
                    entt::entity entity = interest.entity();
                    if (entity == entt::null) {
                        interest.spawn().clear();
                        interest.despawn().clear();
                        continue;
                    }
                    const Transform* t = m_world->registry().try_get<Transform>(entity);
                    if (!t) {
                        interest.spawn().clear();
                        interest.despawn().clear();
                        std::vector<entt::entity> empty;
                        interest.swap_interest(empty);
                        continue;
                    }
                    m_backend->query_neighbors(t->position(), interest.new_interest());
                } else {
                    const auto& b = interest.bounds();
                    m_backend->query_area(b.min, b.max, interest.new_interest());
                }

                std::sort(interest.new_interest().begin(), interest.new_interest().end());

                interest.spawn().clear();
                std::set_difference(
                    interest.new_interest().begin(), interest.new_interest().end(),
                    interest.interest().begin(),     interest.interest().end(),
                    std::back_inserter(interest.spawn())
                );

                interest.despawn().clear();
                std::set_difference(
                    interest.interest().begin(),     interest.interest().end(),
                    interest.new_interest().begin(), interest.new_interest().end(),
                    std::back_inserter(interest.despawn())
                );

                interest.swap_interest(interest.new_interest());
            }
        }
    }
};

} // namespace tw::net::im
