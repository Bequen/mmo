#pragma once

#include <string>
#include <cstdint>

namespace tw {

class WorldEntity {
public:
    std::string name;
    uint32_t entity_id;

    WorldEntity(std::string name, uint32_t entity_id) : 
        name(name),
        entity_id(entity_id)
    {
    }
};

}
