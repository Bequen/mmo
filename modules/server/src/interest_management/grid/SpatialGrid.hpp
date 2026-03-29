#pragma once

#include <vector>

#include <entt/entt.hpp>

namespace tw::net::im::grid {

class SpatialGridCell {
    std::vector<entt::entity> m_entities;

public:
    SpatialGridCell() {

    }

    void append_entity(entt::entity entity) {
        m_entities.push_back(entity);
    }

    void move_entity(entt::entity entity, SpatialGridCell& newCell) {
        auto it = std::find(m_entities.begin(), m_entities.end(), entity);
        if (it != m_entities.end()) {
            m_entities.erase(it);
            newCell.append_entity(entity);
        }
    }
};

class SpatialGrid {
private:
    std::vector<SpatialGridCell> cells;

public:
    SpatialGrid(float cellSize);
};

}
