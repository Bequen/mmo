#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace tw {

class CharacterSystem {
private:
    std::vector<glm::mat4> m_positions;

public:
    /**
     * Character type agnostic function
     */
    void add_character(glm::vec3 initial_position);
};

}
