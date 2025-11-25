#pragma once

#include <glm/glm.hpp>
#include <string>

#include "ByteBuffer.hpp"
#include "Serializers.hpp"
#include "packets/Packet.hpp"

#define MAX_PLAYERS_IN_PACKET 10
#define MAX_PLAYER_NAME_LEN 32

struct PlayerState {
    uint32_t id;
    std::string name;

    glm::vec3 position;
};

struct WorldStateMessage {
    uint32_t frame_idx;
    std::vector<PlayerState> player_states;
};

template<>
class Message<WorldStateMessage> {
public:
    static constexpr PacketType value = WORLD_STATE_PACKET;
};

template<>
class tw::net::Serializer<PlayerState> final {
public:
    static bool serialize(ByteBuffer& buffer, PlayerState& value) {
        buffer.serialize(&value.id);
        buffer.serialize(value.name);
        buffer.serialize(value.position);
        return true;
    }
};


template<>
class tw::net::Serializer<WorldStateMessage> final {
public:
    static bool serialize(ByteBuffer& buffer, WorldStateMessage& value) {
        buffer.serialize(&value.frame_idx);
        VectorSerializer<PlayerState>::serialize(buffer, value.player_states);
        // buffer.serialize(&value.num_players);
        // for(int i = 0; i < value.num_players; i++) {
        //     buffer.serialize(value.player_states[i]);
        // }
        return true;
    }
};
