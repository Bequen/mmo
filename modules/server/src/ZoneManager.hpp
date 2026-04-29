#pragma once

#include "ZoneProxy.hpp"
#include "interest_management/FixedGrid.hpp"
#include "network/SessionId.hpp"
#include "systems/Interest.hpp"
#include "systems/InterestSystem.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/World.hpp"

#include "PlayerMove.pb.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace tw::net {

// 500-unit cells, 500-unit view radius → 3×3 neighbourhood, no per-entity distance checks.
// World bounds are supplied to the constructor at runtime.
using SpatialBackendType = im::FixedGrid<500, 500>;

// How far into this zone a neighbour's interest area is expanded.
// Entities within this margin are tracked by the interest system before transfer.
static constexpr float kNeighborBorderOverlap = 100.0f;

class ZoneManager : public ZoneProxy {
    struct NeighborEntry {
        ZoneProxy*     proxy;
        im::InterestId interest_id;
    };

    // Declaration order == initialisation order.
    std::unique_ptr<World>                                   m_world;
    JoltPhysicsWorld                                         m_physics_world;
    std::unique_ptr<SpatialBackendType>                      m_spatial_backend;
    std::unique_ptr<im::InterestSystem<SpatialBackendType>>  m_interest_system;

    std::unordered_map<im::InterestId, entt::entity>         m_client_entities;
    std::unordered_map<entt::entity, im::InterestId>         m_entity_sessions;  // reverse map
    std::vector<NeighborEntry>                               m_neighbors;
    uint32_t                                                 m_next_neighbor_id = 0x80000000u;

    entt::entity client_entity(im::InterestId interest_id) const;
    void         check_neighbor_transfers();

public:
    entt::registry& registry() { return m_world->registry(); }

    im::InterestSystem<SpatialBackendType>* interest() const {
        return m_interest_system.get();
    }

    ZoneManager(im::AreaBounds zone_bounds);

    // ── ZoneProxy interface ───────────────────────────────────────────────────

    // Returns the geographic area this zone owns, derived from the spatial backend bounds.
    im::AreaBounds area() const override;

    // Receive an entity transferred from a neighbouring zone.
    // Spawns the entity in this zone's world; wires up input routing if session_id is set.
    // NOTE: ZoneServer must also update its session→zone routing table after this call.
    void transfer_entity(EntityInfo&& info) override;

    // ── Zone management ───────────────────────────────────────────────────────

    entt::entity spawn_entity(EntityInfo&& info);

    // Registers an interest for the given entity and begins tracking entities near their position.
    // Uses session_id as the InterestId; the interest system is unaware of this mapping.
    // Can fetch the interest using `get_interest(interest_id)`.
    void add_client(im::InterestId interest_id, entt::entity entity);

    void on_player_move(SessionId session_id, mmo::PlayerMoveMessage&& message);

    // Registers a neighbouring zone. Its geographic area is added to the interest
    // manager and checked each tick; entities that cross into it are transferred.
    void register_neighbor_zone(ZoneProxy* neighbor);

    // Returns the current interest state for any registered id, or nullptr.
    const im::Interest* get_interest(im::InterestId id) const;

    // Runs one tick: interest queries, neighbour transfer checks, world step, physics step.
    void tick(uint32_t frame_idx, float delta_time);
};

} // namespace tw::net
