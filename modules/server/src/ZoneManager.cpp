#include "ZoneManager.hpp"

#include "world/CharacterBody.hpp"
#include "world/CharacterController.hpp"
#include "world/Transform.hpp"
#include "world/WorldEntity.hpp"

#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

namespace tw::net {

ZoneManager::ZoneManager(im::AreaBounds zone_bounds)
    : m_world(std::make_unique<World>()),
      m_physics_world(m_world.get()),
      m_spatial_backend(std::make_unique<SpatialBackendType>(zone_bounds.min.x, zone_bounds.max.x, zone_bounds.min.y, zone_bounds.max.y)),
      m_interest_system(std::make_unique<im::InterestSystem<SpatialBackendType>>(
          m_world.get(), m_spatial_backend.get()
      ))
{}

// ── ZoneProxy interface ───────────────────────────────────────────────────────

im::AreaBounds ZoneManager::area() const {
    return {
        { (float)m_spatial_backend->world_min_x(), (float)m_spatial_backend->world_min_z() },
        { (float)m_spatial_backend->world_max_x(), (float)m_spatial_backend->world_max_z() }
    };
}

void ZoneManager::transfer_entity(EntityInfo&& info) {
    spdlog::info("Transfered entity to zone ({} {}) ({} {})", area().min.x, area().min.y, area().max.x, area().max.y);
    spawn_entity(std::move(info));
}

// ── Zone management ───────────────────────────────────────────────────────────

entt::entity ZoneManager::spawn_entity(EntityInfo&& info) {
    entt::entity entity = m_world->registry().create();

    m_world->registry().emplace<WorldEntity>(entity, info.name, (uint32_t)entity);
    m_world->registry().emplace<Transform>(entity, Transform(info.position));
    m_world->registry().emplace<CharacterController>(entity, 20.0f);
    m_world->registry().emplace<CharacterBody>(
        entity,
        m_physics_world.create_character(
            new JPH::BoxShape(JPH::Vec3Arg(0.5f, 0.5f, 0.5f)),
            info.position
        )
    );

    return entity;
}

void ZoneManager::add_client(im::InterestId interest_id, entt::entity entity) {
    m_client_entities[interest_id] = entity;
    m_entity_sessions[entity]      = interest_id;
    m_interest_system->set_entity_interest(interest_id, entity);
}

entt::entity ZoneManager::client_entity(im::InterestId interest_id) const {
    auto it = m_client_entities.find(interest_id);
    return it != m_client_entities.end() ? it->second : entt::null;
}

void ZoneManager::on_player_move(SessionId session_id, mmo::PlayerMoveMessage&& message) {
    ZoneScoped;
    entt::entity entity = client_entity(session_id);
    if (entity == entt::null) return;

    auto* controller = m_world->registry().try_get<CharacterController>(entity);
    if (controller) {
        controller->set_input(
            message.frame_idx(),
            glm::vec3(message.input().x(), message.input().y(), message.input().z())
        );
    } else {
        spdlog::error("Player {} has no character controller", (uint32_t)entity);
    }
}

void ZoneManager::register_neighbor_zone(ZoneProxy* neighbor) {
    im::InterestId id = m_next_neighbor_id++;
    m_neighbors.push_back({ neighbor, id });

    // Expand bounds so entities approaching the border appear in the interest
    // system before they cross. Transfer still uses the true (unexpanded) bounds.
    auto b = neighbor->area();
    im::AreaBounds expanded {
        { b.min.x - kNeighborBorderOverlap, b.min.y - kNeighborBorderOverlap },
        { b.max.x + kNeighborBorderOverlap, b.max.y + kNeighborBorderOverlap }
    };
    m_interest_system->set_area_interest(id, expanded);
}

const im::Interest* ZoneManager::get_interest(im::InterestId id) const {
    return m_interest_system->get_interest(id);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void ZoneManager::check_neighbor_transfers() {
    ZoneScopedN("ZoneManager::check_neighbor_transfers");

    for (const auto& [proxy, interest_id] : m_neighbors) {
        const im::Interest* interest = m_interest_system->get_interest(interest_id);
        if (!interest || interest->interest().empty()) continue;

        // Only transfer entities that have actually crossed into the neighbour's
        // true area (not just the expanded interest margin).
        auto true_bounds = proxy->area();

        std::vector<entt::entity> to_transfer;
        for (entt::entity entity : interest->interest()) {
            const Transform* transform = m_world->registry().try_get<Transform>(entity);
            if (!transform) continue;

            glm::vec3 pos = transform->position();
            if (pos.x >= true_bounds.min.x && pos.x <= true_bounds.max.x &&
                pos.z >= true_bounds.min.y && pos.z <= true_bounds.max.y) {
                to_transfer.push_back(entity);
            }
        }

        for (entt::entity entity : to_transfer) {
            auto* world_entity = m_world->registry().try_get<WorldEntity>(entity);
            auto* transform    = m_world->registry().try_get<Transform>(entity);
            if (!world_entity || !transform) continue;

            EntityInfo info {
                .name     = world_entity->name,
                .position = transform->position()
            };

            // Remove client tracking on the sending side.
            // The owner (e.g. ZoneServer) is responsible for calling add_client on
            // the receiving zone with the correct session mapping.
            auto session_it = m_entity_sessions.find(entity);
            if (session_it != m_entity_sessions.end()) {
                spdlog::info(
                    "Transferring interest {} entity {} to neighbour zone",
                    session_it->second, (uint32_t)entity
                );
                m_interest_system->remove_interest(session_it->second);
                m_client_entities.erase(session_it->second);
                m_entity_sessions.erase(session_it);
            }

            proxy->transfer_entity(std::move(info));

            // TODO: notify physics world to release the Jolt CharacterVirtual.
            m_world->registry().destroy(entity);
        }
    }
}

// ── Tick ──────────────────────────────────────────────────────────────────────

void ZoneManager::tick(uint32_t frame_idx, float delta_time) {
    ZoneScopedN("ZoneManager::tick");

    FrameMarkStart("Interest System");
    m_interest_system->update();
    FrameMarkEnd("Interest System");

    check_neighbor_transfers();

    FrameMarkStart("World step");
    m_world->step(delta_time);
    FrameMarkEnd("World step");

    FrameMarkStart("Physics step");
    m_physics_world.step(frame_idx, delta_time);
    FrameMarkEnd("Physics step");
}

} // namespace tw::net
