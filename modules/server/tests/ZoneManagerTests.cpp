#include "ZoneManager.hpp"
#include <cassert>

using namespace tw::net;

int main() {
    ZoneManager zoneA(im::AreaBounds(
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1000.0f, 1000.0f)
    ));

    ZoneManager zoneB(im::AreaBounds(
        glm::vec2(1000.0f, 0.0f),
        glm::vec2(2000.0f, 1000.0f)
    ));

    zoneA.register_neighbor_zone(&zoneB);
    zoneB.register_neighbor_zone(&zoneA);

    auto entityA = zoneA.spawn_entity({
        .name = "EntityA",
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
    });
    zoneA.add_client(1, entityA);


    auto entityB = zoneB.spawn_entity({
        .name = "EntityB",
        .position = glm::vec3(1030.0f, 500.0f, 0.0f),
    });
    zoneB.add_client(1, entityB);

    auto transformB = zoneB.registry().get<tw::Transform>(entityB);

    auto interest = zoneA.get_interest(1);

    assert(interest->interest().empty());

    tw::Transform* transformA = zoneA.registry().try_get<tw::Transform>(entityA);
    transformA->set_position(glm::vec3(970.0f, 0.0f, 0.0f));

    zoneA.tick(0, 0.1f);
    zoneB.tick(0, 0.1f);

    interest = zoneA.get_interest(1);

    transformA = zoneA.registry().try_get<tw::Transform>(entityA);
    transformA->set_position(glm::vec3(1020.0f, 0.0f, 0.0f));

    zoneA.tick(1, 0.1f);
    zoneB.tick(1, 0.1f);

    interest = zoneA.get_interest(1);

    return 0;
}
