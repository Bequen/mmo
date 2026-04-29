#pragma once

#include <concepts>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace tw::net::im {

template<typename Backend>
concept SpatialBackend = requires(
    Backend& backend,
    entt::entity entity,
    glm::vec3 position,
    glm::vec2 area_min,
    glm::vec2 area_max,
    std::vector<entt::entity>& out
) {
    { backend.begin_frame() } -> std::same_as<void>;
    { backend.insert(entity, position) } -> std::same_as<void>;
    { backend.query_neighbors(position, out) } -> std::same_as<void>;
    // Returns all entities in cells that overlap [area_min, area_max] (XZ plane).
    // May include entities in cells that partially extend beyond the AABB boundary.
    { backend.query_area(area_min, area_max, out) } -> std::same_as<void>;
};

}
