#pragma once

#include <glm/gtc/quaternion.hpp>

#include "world/Camera.hpp"
#include "io/InputState.hpp"

namespace tw {

/**
 * Player controller from third person
 */
class ThirdPersonPlayerController {
    Camera* m_camera;
    glm::vec3 m_input;

    glm::quat m_camera_rotation;
    float m_camera_zoom;
    float m_sensitivity;

    glm::vec3 m_target;

    glm::vec3 get_target_position() {
        return m_target;
    }

public:
    GET_REF(m_input, input);

    void set_target(glm::vec3 target) {
        m_target = target;
    }

    ThirdPersonPlayerController(Camera* camera, glm::vec3 target);

    glm::vec3 get_axis();

    void update(const tw::io::InputManager* input, double delta_time);
};

}
