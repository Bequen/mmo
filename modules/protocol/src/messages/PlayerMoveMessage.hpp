#pragma once

#include <nlohmann/json.hpp>

#include "Serialization.hpp"
#include "packets/Packet.hpp"
#include "Serializers.hpp"
#include "GlmSerializers.hpp"
#include <glm/glm.hpp>
#include <vector>

MESSAGE(PlayerInputMessage, PLAYER_UPDATE_MSG) {
public:
    uint32_t frame_idx;
    uint32_t entity_id;
    glm::vec3 inputs;
};

MESSAGE(PlayerJoinRequestMessage, PLAYER_JOIN_REQUEST_PACKET) {
public:
    std::string name;
};

MESSAGE(PlayerJoinResponseMessage, LOGIN_RESPONSE_PACKET) {
public:
    bool is_valid;
    uint32_t entity_id;
    glm::vec3 position;
};

MESSAGE(EntitySpawnMessage, ENTITY_SPAWN_MSG) {
public:
    uint32_t entity_id;
    bool is_player; // could determine what commands should the entity get
    std::string name;
    glm::vec3 position;
};

MESSAGE(EntityDespawnMessage, ENTITY_DESPAWN_MSG) {
public:
    uint32_t entity_id;
    bool is_player; // could determine what commands should the entity get
};


struct EntityPosition {
    uint32_t entity_id;
    glm::vec3 position;
};

MESSAGE(EntityMoveMessage, ENTITY_MOVE_PACKET) {
public:
    uint32_t frame_idx;
    std::vector<EntityPosition> entities;
};

template<>
class tw::net::Serializer<PlayerInputMessage> {
public:
    static bool serialize(Serialization& buffer, PlayerInputMessage& message) {
        buffer.serialize(&message.frame_idx);
        buffer.serialize(&message.entity_id);
        buffer.serialize(message.inputs);
        return true;
    }
};

// inline void to_json(json& j, const PlayerInputMessage& value) {
//     j = json{
//         {"frame_idx", value.frame_idx},
//         {"entity_id", value.entity_id}
//     };
// }


template<>
class tw::net::Serializer<EntitySpawnMessage> {
public:
    static bool serialize(Serialization& buffer, EntitySpawnMessage& message) {
        buffer.serialize(&message.entity_id);
        buffer.serialize(&message.is_player);
        buffer.serialize(message.name);
        // buffer.serialize(message.position);
        return true;
    }
};

template<>
class tw::net::Serializer<EntityDespawnMessage> {
public:
    static bool serialize(Serialization& buffer, EntityDespawnMessage& message) {
        buffer.serialize(&message.entity_id);
        buffer.serialize(&message.is_player);
        return true;
    }
};

template<>
class tw::net::Serializer<EntityPosition> {
public:
    static bool serialize(Serialization& buffer, EntityPosition& message) {
        buffer.serialize(&message.entity_id);
        // buffer.serialize(message.position);
        return true;
    }
};

template<>
class tw::net::Serializer<EntityMoveMessage> {
public:
    static bool serialize(Serialization& buffer, EntityMoveMessage& message) {
        buffer.serialize(&message.frame_idx);
        VectorSerializer<EntityPosition>::serialize(buffer, message.entities);
        return true;
    }
};


template<>
class tw::net::Serializer<PlayerJoinRequestMessage> {
public:
    static bool serialize(Serialization& buffer, PlayerJoinRequestMessage& message) {
        buffer.serialize(message.name);
        return true;
    }
};

// inline void to_json(json& j, const PlayerJoinRequestMessage& value) {
//     j = json{
//         {"name", value.name}
//     };
// }

template<>
class tw::net::Serializer<PlayerJoinResponseMessage> {
public:
    static bool serialize(Serialization& buffer, PlayerJoinResponseMessage& message) {
        buffer.serialize(&message.is_valid);
        buffer.serialize(&message.entity_id);
        // buffer.serialize(message.position);
        return true;
    }
};
