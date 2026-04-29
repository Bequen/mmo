#pragma once

#include "WorldState.pb.h"
#include "Chat.pb.h"
#include "Cluster.pb.h"
#include "Login.pb.h"
#include "PlayerMove.pb.h"
#include "Entity.pb.h"

#include <spdlog/fmt/fmt.h>
#include <string_view>

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
    PLAYER_JOIN_RESPONSE_PACKET,

    CHAT_SEND_MESSAGE_REQUEST,
    CHAT_SEND_MESSAGE_RESPONSE,
    CHAT_JOIN_CHANNEL_REQUEST,
    CHAT_JOIN_CHANNEL_RESPONSE,
    CHAT_LEAVE_CHANNEL_REQUEST,
    CHAT_LEAVE_CHANNEL_RESPONSE,
    CHAT_MESSAGE_BROADCAST_REQUEST,

    CLUSTER_ZONE_HELLO,
    CLUSTER_ZONE_BYE,
};

namespace tw {
template<typename Msg>
class Message;


template<>
class Message<mmo::LoginRequest> {
public:
    static constexpr PacketType value = LOGIN_REQUEST_MSG;
};

template<>
class Message<mmo::LoginResponse> {
public:
    static constexpr PacketType value = LOGIN_RESPONSE_PACKET;
};

template<>
class Message<mmo::WorldStateMessage> {
public:
    static constexpr PacketType value = WORLD_STATE_PACKET;
};

template<>
class Message<mmo::PlayerMoveMessage> {
public:
    static constexpr PacketType value = PLAYER_UPDATE_MSG;
};

template<>
class Message<mmo::EntitySpawnMessage> {
public:
    static constexpr PacketType value = ENTITY_SPAWN_MSG;
};

template<>
class Message<mmo::EntityDespawnMessage> {
public:
    static constexpr PacketType value = ENTITY_DESPAWN_MSG;
};


template<>
class Message<mmo::chat::SendChatMessageRequest> {
public:
    static constexpr PacketType value = CHAT_SEND_MESSAGE_REQUEST;
};

template<>
class Message<mmo::chat::SendChatMessageResponse> {
public:
    static constexpr PacketType value = CHAT_SEND_MESSAGE_RESPONSE;
};

template<>
class Message<mmo::chat::JoinChannelRequest> {
public:
    static constexpr PacketType value = CHAT_JOIN_CHANNEL_REQUEST;
};

template<>
class Message<mmo::chat::JoinChannelResponse> {
public:
    static constexpr PacketType value = CHAT_JOIN_CHANNEL_RESPONSE;
};

template<>
class Message<mmo::chat::LeaveChannelRequest> {
public:
    static constexpr PacketType value = CHAT_LEAVE_CHANNEL_REQUEST;
};

template<>
class Message<mmo::chat::LeaveChannelResponse> {
public:
    static constexpr PacketType value = CHAT_LEAVE_CHANNEL_RESPONSE;
};

template<>
class Message<mmo::chat::ChatMessageBroadcastRequest> {
public:
    static constexpr PacketType value = CHAT_MESSAGE_BROADCAST_REQUEST;
};

template<>
class Message<mmo::cluster::ZoneHello> {
public:
    static constexpr PacketType value = CLUSTER_ZONE_HELLO;
};

template<>
class Message<mmo::cluster::ZoneBye> {
public:
    static constexpr PacketType value = CLUSTER_ZONE_BYE;
};
}


template<>
struct fmt::formatter<PacketType> : fmt::formatter<std::string_view> {
    auto format(PacketType type, fmt::format_context& ctx) const {
        std::string_view name;
        switch (type) {
            case MESSAGE_PACKET:               name = "MESSAGE_PACKET";               break;
            case LOGIN_REQUEST_MSG:            name = "LOGIN_REQUEST_MSG";            break;
            case LOGIN_RESPONSE_PACKET:        name = "LOGIN_RESPONSE_PACKET";        break;
            case WORLD_STATE_PACKET:           name = "WORLD_STATE_PACKET";           break;
            case PLAYER_UPDATE_MSG:            name = "PLAYER_UPDATE_MSG";            break;
            case PLAYER_UPDATE_PACKET:         name = "PLAYER_UPDATE_PACKET";         break;
            case ENTITY_SPAWN_MSG:             name = "ENTITY_SPAWN_MSG";             break;
            case ENTITY_DESPAWN_MSG:           name = "ENTITY_DESPAWN_MSG";           break;
            case ENTITY_MOVE_PACKET:           name = "ENTITY_MOVE_PACKET";           break;
            case PLAYER_JOIN_REQUEST_PACKET:   name = "PLAYER_JOIN_REQUEST_PACKET";   break;
            case PLAYER_JOIN_RESPONSE_PACKET:  name = "PLAYER_JOIN_RESPONSE_PACKET";  break;
            case CHAT_SEND_MESSAGE_REQUEST:            name = "CHAT_SEND_MESSAGE";            break;
            case CHAT_JOIN_CHANNEL_REQUEST:            name = "CHAT_JOIN_CHANNEL";            break;
            case CHAT_LEAVE_CHANNEL_REQUEST:           name = "CHAT_LEAVE_CHANNEL";           break;
            case CHAT_MESSAGE_BROADCAST_REQUEST:       name = "CHAT_MESSAGE_BROADCAST";       break;
            case CLUSTER_ZONE_HELLO:                   name = "CLUSTER_ZONE_HELLO";           break;
            case CLUSTER_ZONE_BYE:                     name = "CLUSTER_ZONE_BYE";             break;
            default:                           name = "UNKNOWN";                      break;
        }
        return fmt::formatter<std::string_view>::format(name, ctx);
    }
};
