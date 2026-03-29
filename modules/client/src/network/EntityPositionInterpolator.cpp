#include "EntityPositionInterpolator.hpp"

#include "world/Transform.hpp"
#include "InterpolatedProperty.hpp"
#include "EntityInterpolation.hpp"

#include <spdlog/spdlog.h>

namespace tw::net {

EntityPositionInterpolator::EntityPositionInterpolator(
    entt::registry* registry, entt::entity player_entity, size_t bufferingIntervalInMillis
) : m_registry(registry),
    m_player_entity(player_entity),
    m_bufferingIntervalInMillis(bufferingIntervalInMillis)
{

}

void EntityPositionInterpolator::register_entity(entt::entity entity) {
    m_registry->emplace<EntityPositionInterpolation>(entity, glm::vec3());
}

void EntityPositionInterpolator::add_position_for_entity(entt::entity entity, glm::vec3 position) {
    auto* interpolation = m_registry->try_get<EntityPositionInterpolation>(entity);
    if(interpolation == nullptr) {
        spdlog::warn("Attempt to add position for non-registered entity {}", (uint32_t)entity);
        return;
    }

    interpolation->push(Clock::now(), position);
}

glm::vec3 EntityPositionInterpolator::get_position_for_entity(entt::entity entity) {
    auto* interpolation = m_registry->try_get<EntityPositionInterpolation>(entity);
    if(interpolation == nullptr) {
        spdlog::warn("Attempt to get position for non-registered entity {}", (uint32_t)entity);
        return glm::vec3();
    }

    auto now = Clock::now() - std::chrono::milliseconds(m_bufferingIntervalInMillis);

    auto [from, to, value] = interpolation->get_values_around(now);
    return glm::mix(from, to, value);
}

void EntityPositionInterpolator::update() {
    m_registry->view<EntityPositionInterpolation, Transform>()
        .each([&](const auto entity, const EntityPositionInterpolation& interpolation, Transform& ts) {
            auto now = Clock::now() - std::chrono::milliseconds(m_bufferingIntervalInMillis);

            auto [from, to, value] = interpolation.get_values_around(now);
            ts.set_position(glm::mix(from, to, value));
        });
}

}
