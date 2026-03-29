#pragma once

#include <string>

#include "Serialization.hpp"

namespace tw::net {

#define SERIALIZER(class_name, buffer, message) \
    template<> bool tw::net::Serializer<class_name>::serialize(Serialization& buffer, class_name& message)

template<typename T>
class VectorSerializer final {
public:
    static bool serialize(tw::net::Serialization& buffer, std::vector<T>& values) {
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
class Serializer<std::string> final {
public:
    static bool serialize(tw::net::Serialization& buffer, std::string& value) {
        uint32_t length = value.length();
        buffer.serialize(&length);

        if(buffer.is_reading()) {
            value = std::string();
            value.resize(length);
        }

        buffer.serialize(value.data(), length);

        return true;
    }
};

}
