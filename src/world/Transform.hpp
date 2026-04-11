#pragma once

#include "glm/common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#include <spdlog/spdlog.h>

namespace tw {

struct Transform {
    glm::mat4 transform;

public:
    Transform(glm::vec3 translation) :
        transform(glm::translate(glm::mat4(1.0f), translation))
    { }

    Transform(glm::mat4 transform) :
        transform(transform)
    { }

    void set_position(glm::vec3 position) {
        transform = glm::translate(glm::mat4(1.0f), position);
    }

    void translate(glm::vec3 translation) {
        transform = glm::translate(transform, translation);
    }

    void look_at(glm::vec3 eye, glm::vec3 target) {
        transform = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    inline constexpr glm::vec3 position() const {
        return glm::vec3(transform[3][0], transform[3][1], transform[3][2]) * transform[3][3];
    }

    constexpr glm::vec3 right() const {
        return glm::vec3(transform[2][0], transform[2][1], transform[2][2]) * transform[2][3];
    }

    constexpr bool is_closer_than(const Transform& other, float max_distance) const {
        auto from = position();
        auto to = other.position();

        auto vector = to - from;

        return vector.x * vector.x +
            vector.y * vector.y +
            vector.z * vector.z <
            max_distance * max_distance;
    }
};

}
