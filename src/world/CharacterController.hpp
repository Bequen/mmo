#pragma once

#include <glm/glm.hpp>

#include "HistoryBuffer.hpp"

namespace tw {

/**
 * Controls the character's body
 */
class CharacterController {
    float m_speed;

    HistoryBuffer<uint32_t, glm::vec3, 20 * 10> m_history;
    HistoryBuffer<uint32_t, glm::vec3, 20 * 10> m_position_history;

public:
    GET(m_speed, speed);
    GET_MUT_REF(m_history, input_history);
    GET_MUT_REF(m_position_history, position_history);

    CharacterController(float speed) : 
        m_speed(speed),
        m_history(0, glm::vec3()),
        m_position_history(0, glm::vec3())
    { }

    void set_input(uint32_t frame_idx, glm::vec3 input) {
        m_history.set(frame_idx, input);
    }

    glm::vec3 input() {
        return m_history.values()[m_history.values().size() - 1];
    }

    glm::vec3 input(uint32_t frame_idx) {
        auto value = m_history.get(frame_idx);
        if(value.has_value()) {
            return *value.value();
        }

        return glm::vec3();
    }
};

}
