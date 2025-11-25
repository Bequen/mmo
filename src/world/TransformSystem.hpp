#pragma once

class TransformSystem {
    std::vector<glm::mat4> m_positions;

public:

    void add_transform(EntityId entity);
};
