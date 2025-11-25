#include "Camera.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace tw {

Camera::Camera(float aspect_ratio) :
m_data( glm::perspective(40.0f, aspect_ratio, 0.1f, 1000.0f),
        Transform(glm::vec3(10.0f, 10.0f, 10.0f))) {
}

}
