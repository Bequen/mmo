#pragma once

#include <glm/glm.hpp>

#include "Messenger.hpp"

namespace tw::net {

class PlayerClient {
public:
    uint32_t m_id;
    Messenger m_messenger;

    std::string m_name;
    uint32_t m_entity_id;
    
    uint32_t m_last_frame_idx;
    bool m_frame_responded = true;

    std::array<glm::vec3, 3> m_interpolation_buffer;
    uint32_t m_first_time;
    uint32_t m_num_times;

    PlayerClient(uint32_t id, Messenger&& messenger, std::string name) :
        m_id(id),
        m_entity_id(id),
        m_messenger(std::move(messenger)),
        m_name(name),
        m_last_frame_idx(0)
    {
    }

    void set_last_frame_idx(uint32_t frame_idx) {
        if(frame_idx > m_last_frame_idx) {
            m_last_frame_idx = frame_idx;
            m_frame_responded = false;
        }
    }
};

}
