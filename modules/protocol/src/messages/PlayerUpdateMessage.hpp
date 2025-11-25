#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#include "Serializers.hpp"
#include "packets/Packet.hpp"

struct PlayerUpdate {
    uint32_t id;
    glm::vec3 position;
};

SERIALIZER(PlayerUpdate, buffer, update) {
    buffer.serialize(&update.id);
    buffer.serialize(update.position);
    return true;
}

#define MAX_PLAYER_UPDATES 10

MESSAGE(PlayerUpdateMessage, PLAYER_UPDATE_PACKET) {
public:
    uint32_t update_count;
    PlayerUpdate updates[MAX_PLAYER_UPDATES];
};

SERIALIZER(PlayerUpdateMessage, buffer, mesg) {
    buffer.serialize(&mesg.update_count);
    for(int i = 0; i < mesg.update_count; i++) {
        buffer.serialize(mesg.updates[i]);
    }

    return true;
}

