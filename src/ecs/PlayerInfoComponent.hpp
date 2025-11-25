#pragma once

#include <string>
#include <cstdint>

struct PlayerInfoComponent {
    uint32_t entity_id;
    std::string name;

    PlayerInfoComponent(uint32_t entity_id, std::string name) :
        entity_id(entity_id),
        name(name)
    { }
};
