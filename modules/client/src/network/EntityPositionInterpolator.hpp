
#include <chrono>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace tw::net {


class EntityPositionInterpolator {

    using Clock = std::chrono::high_resolution_clock;

    entt::registry* m_registry;

    entt::entity m_player_entity;

    size_t m_bufferingIntervalInMillis;

public:
    EntityPositionInterpolator(
        entt::registry* registry,
        entt::entity player_entity,
        size_t bufferingIntervalInMillis);

    void register_entity(entt::entity entity);

    void add_position_for_entity(entt::entity entity, glm::vec3 position);

    glm::vec3 get_position_for_entity(entt::entity entity);

    void update();
};

}
