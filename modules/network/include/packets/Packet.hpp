#pragma once

#include "Serializers.hpp"

#define PACKET(name) struct #name {

enum PacketType {
    MESSAGE_PACKET,
    LOGIN_REQUEST_MSG,
    LOGIN_RESPONSE_PACKET,
    WORLD_STATE_PACKET,
    PLAYER_UPDATE_MSG,
    PLAYER_UPDATE_PACKET,
    ENTITY_SPAWN_MSG,
    ENTITY_DESPAWN_MSG,
    ENTITY_MOVE_PACKET,
    PLAYER_JOIN_REQUEST_PACKET,
    PLAYER_JOIN_RESPONSE_PACKET
};

template<typename Msg>
class Message;

#define MESSAGE(class_name, message_type_id) class class_name; \
    template<> \
    class Message<class_name> { \
        public: static constexpr PacketType value = message_type_id; \
    }; \
\
class class_name

template<>
class tw::net::Serializer<const PacketType> final {
public:
    static bool serialize(tw::net::Serialization& buffer, const PacketType& value) {
        uint32_t v = value;
        return buffer.serialize((uint32_t*)&v);
    }
};


template<>
class tw::net::Serializer<PacketType> final {
public:
    static bool serialize(tw::net::Serialization& buffer, PacketType& value) {
        return buffer.serialize((uint32_t*)&value);
    }
};
