#pragma once

#include <glm/glm.hpp>
#include <string>

#include "ByteBuffer.hpp"

template<typename T>
class VectorSerializer final {
public:
    static bool serialize(tw::net::ByteBuffer& buffer, std::vector<T>& values) {
        uint32_t size = values.size();
        buffer.serialize(&size);

        if(buffer.is_reading()) {
            values = std::vector<T>(size);
        }

        for(int i = 0; i < values.size(); i++) {
            buffer.serialize(values[i]);
        }

        return true;
    }
};

template<>
class tw::net::Serializer<std::string> final {
public:
    static bool serialize(ByteBuffer& buffer, std::string& value) {
        uint32_t length = value.length();
        buffer.serialize(&length);

        if(buffer.is_reading()) {
            value = std::string();
            value.resize(length);
        }

        buffer.bytes(value.data(), length);

        return true;
    }
};


template<>
class tw::net::Serializer<glm::vec3> final {
public:
    static bool serialize(ByteBuffer& buffer, glm::vec3& value) {
        buffer.serialize(&value.x);
        buffer.serialize(&value.y);
        buffer.serialize(&value.z);
        return true;
    }
};
