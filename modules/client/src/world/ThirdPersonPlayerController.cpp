#include "ThirdPersonPlayerController.hpp"

#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace tw {


ThirdPersonPlayerController::ThirdPersonPlayerController(Camera* camera, glm::vec3 target) :
    m_camera(camera),
    m_camera_rotation(1.0f, 0.0f, 0.0f, 0.0f),
    m_camera_zoom(10.0f),
    m_sensitivity(0.01f),
    m_target(target),
    m_input(0.0f, 0.0f, 0.0f)
{

}

glm::vec3 ThirdPersonPlayerController::get_axis() {
    glm::vec3 forward = m_camera_rotation * glm::vec3(0, 0, -1);
    forward.y = 0.0f;
    return glm::normalize(forward);
}
void ThirdPersonPlayerController::update(const tw::io::InputManager* input, double delta_time) {
    // glm::vec3 velocity = {
    //     input->axis_x(),
    //     0.0f,
    //     input->axis_y()
    // };

    // velocity *= m_player_body->m_speed * delta_time;
    glm::vec3 forward = m_camera_rotation * glm::vec3(0, 0, -1);
    forward.y = 0.0f;
    forward = glm::normalize(forward);

    glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 velocity = forward * input->axis_y() + right * -input->axis_x();

    if(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z > 0.0f) {
        m_input = glm::normalize(velocity);
    } else {
        m_input = glm::vec3(0.0f);
    }

    glm::quat yaw = glm::angleAxis(-input->motion_x() * m_sensitivity, glm::vec3(0, 1, 0));
    glm::quat pitch = glm::angleAxis(-input->motion_y() * m_sensitivity, glm::vec3(1, 0, 0));

    m_camera_rotation = yaw * m_camera_rotation * pitch;

    glm::vec3 offset = m_camera_rotation * glm::vec3(0, 0, m_camera_zoom);
    m_camera->view().look_at(get_target_position() + offset, get_target_position());
}

}
