#pragma once

#include <chrono>
#include <glm/glm.hpp>

#include "metrics/HistoryBuffer.hpp"

namespace tw {

/**
 * Controls the character's body
 */
class CharacterController {
    float m_speed;

    using Clock = std::chrono::steady_clock;

    HistoryBuffer<Clock::time_point, glm::vec3> m_history;
    HistoryBuffer<Clock::time_point, glm::vec3> m_position_history;

    glm::vec3 m_input;

    uint32_t m_frame_idx;

public:
    GET(m_speed, speed);
    GET_MUT_REF(m_history, input_history);
    GET_MUT_REF(m_position_history, position_history);

    CharacterController(float speed) :
        m_speed(speed),
        m_history(Clock::now(), glm::vec3(), 10 * 20),
        m_position_history(Clock::now(), glm::vec3(), 10 * 20),
        m_frame_idx(0)
    { }

    void set_input(uint32_t frame_idx, glm::vec3 input) {
        // m_history.set(frame_idx, input);
        m_input = input;
    }

    void set_frame_idx(uint32_t idx) {
        m_frame_idx = idx;
    }

    glm::vec3 input() const {
        return m_input;
        // return m_history.values()[m_history.values().size() - 1];
    }

    glm::vec3 input(uint32_t frame_idx) const {
        return m_input;
        // auto value = m_history.get(frame_idx);
        // if(value.has_value()) {
        //     return *value.value();
        // }

        // return glm::vec3();
    }
};

}
