#pragma once

#include "ByteBuffer.hpp"
#include "packets/Packet.hpp"
#include "Serializers.hpp"
#include <glm/glm.hpp>
#include <vector>

MESSAGE(PlayerInputMessage, PLAYER_MOVE_PACKET) {
public:
    uint32_t frame_idx;
    uint32_t entity_id;
    glm::vec3 inputs;
};

MESSAGE(PlayerJoinRequestMessage, PLAYER_JOIN_REQUEST_PACKET) {
public:
    std::string name;
};

MESSAGE(PlayerJoinResponseMessage, PLAYER_JOIN_RESPONSE_PACKET) {
public:
    bool is_valid;
    uint32_t entity_id;
    glm::vec3 position;
};

MESSAGE(EntitySpawnMessage, ENTITY_SPAWN_PACKET) {
public:
    uint32_t entity_id;
    bool is_player; // could determine what commands should the entity get
    std::string name;
    glm::vec3 position;
};

MESSAGE(EntityDespawnMessage, ENTITY_DESPAWN_PACKET) {
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
    static bool serialize(ByteBuffer& buffer, PlayerInputMessage& message) {
        buffer.serialize(&message.frame_idx);
        buffer.serialize(&message.entity_id);
        buffer.serialize(message.inputs);
        return true;
    }
};


template<>
class tw::net::Serializer<EntitySpawnMessage> {
public:
    static bool serialize(ByteBuffer& buffer, EntitySpawnMessage& message) {
        buffer.serialize(&message.entity_id);
        buffer.serialize(&message.is_player);
        buffer.serialize(message.name);
        buffer.serialize(message.position);
        return true;
    }
};

template<>
class tw::net::Serializer<EntityDespawnMessage> {
public:
    static bool serialize(ByteBuffer& buffer, EntityDespawnMessage& message) {
        buffer.serialize(&message.entity_id);
        buffer.serialize(&message.is_player);
        return true;
    }
};

template<>
class tw::net::Serializer<EntityPosition> {
public:
    static bool serialize(ByteBuffer& buffer, EntityPosition& message) {
        buffer.serialize(&message.entity_id);
        buffer.serialize(message.position);
        return true;
    }
};

template<>
class tw::net::Serializer<EntityMoveMessage> {
public:
    static bool serialize(ByteBuffer& buffer, EntityMoveMessage& message) {
        buffer.serialize(&message.frame_idx);
        VectorSerializer<EntityPosition>::serialize(buffer, message.entities);
        return true;
    }
};


template<>
class tw::net::Serializer<PlayerJoinRequestMessage> {
public:
    static bool serialize(ByteBuffer& buffer, PlayerJoinRequestMessage& message) {
        std::println("Serializing player {}", message.name);
        buffer.serialize(message.name);
        return true;
    }
};


template<>
class tw::net::Serializer<PlayerJoinResponseMessage> {
public:
    static bool serialize(ByteBuffer& buffer, PlayerJoinResponseMessage& message) {
        buffer.serialize(&message.is_valid);
        buffer.serialize(&message.entity_id);
        buffer.serialize(message.position);
        return true;
    }
};
