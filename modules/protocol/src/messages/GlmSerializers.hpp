#pragma once

#include <glm/glm.hpp>
#include <string>
#include "Serializers.hpp"

template<>
class tw::net::Serializer<glm::vec3> {
public:
    static bool serialize(Serialization& buffer, glm::vec3& value) {
        buffer.serialize(&value.x);
        buffer.serialize(&value.y);
        buffer.serialize(&value.z);
        return true;
    }
};
