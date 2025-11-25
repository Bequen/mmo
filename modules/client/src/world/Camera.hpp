#pragma once

#include "common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/glm.hpp>
#include "world/Transform.hpp"

namespace tw {

struct CameraData {
    glm::mat4 projection;
    Transform view;

    CameraData(glm::mat4 projection, Transform view) :
        projection(projection),
        view(view)
    {
    }
};

class Camera {
    CameraData m_data;

public:
    GET_MUT_REF(m_data.view, view);

    glm::vec3 position() const {
        return m_data.view.position();
    }

    void look_at(glm::vec3 from, glm::vec3 to) {
        m_data.view = glm::lookAt(from, to, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    /**
     * For uploading data to the GPU buffer
     */
    inline const CameraData* data() const { return &m_data; }

    /**
     * 
     */
    Camera(float aspect_ratio);
};

}
