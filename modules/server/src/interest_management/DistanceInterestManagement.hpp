#pragma once

#include "messages/PlayerMoveMessage.hpp"
#include "world/World.hpp"
#include "world/Transform.hpp"

namespace tw::server::im {

template<typename T>
class InterestResult {
public:
    const std::vector<T> m_entities;

    InterestResult(const std::vector<T> entities) :
        m_entities(entities){
    }
};

template<typename T>
class DistanceInterestManagement {
private:
    const World* m_world;
    
    const float m_view_distance;

public:
    DistanceInterestManagement(const World* world, float view_distance) :
        m_world(world),
        m_view_distance(view_distance)
    {
    }

    InterestResult<EntityPosition> query(entt::entity entity) {
        const Transform* ts = m_world->registry().try_get<Transform>(entity);

        std::vector<EntityPosition> entities;
        m_world->registry().view<Transform>()
            .each([&](const auto e, const Transform& t) {
                    if(true/* && ts->is_closer_than(t, m_view_distance) */) {
                        entities.push_back(EntityPosition {
                            .entity_id = (uint32_t)e,
                            .position = t.position()
                        });
                    }
            });

        return {entities};
    }
};

}
