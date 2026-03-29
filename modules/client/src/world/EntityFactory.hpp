#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace tw {

class EntityFactory {
public:
    entt::entity create_player_entity(glm::vec3 position);
};

}
