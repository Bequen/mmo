#include "CharacterSystem.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace tw {

void CharacterSystem::add_character(glm::vec3 initial_position) {
    glm::mat4 transform = glm::translate(glm::mat4(), initial_position);
    m_positions.push_back(transform);
}

}
