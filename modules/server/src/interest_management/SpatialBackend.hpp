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
    std::vector<entt::entity>& out
) {
    { backend.begin_frame() } -> std::same_as<void>;
    { backend.insert(entity, position) } -> std::same_as<void>;
    { backend.query_neighbors(position, out) } -> std::same_as<void>;
};

}
