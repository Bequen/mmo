#pragma once

#include "common.hpp"

#include "entt/entt.hpp"

namespace tw {

class World {
protected:
    entt::registry m_registry;

public:
    GET_MUT_REF(m_registry, registry);

    World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void step(double delta_time);
};

}
