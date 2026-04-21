#pragma once

#include "Entity.pb.h"
#include "PlayerMove.pb.h"
#include "WorldState.pb.h"
#include "Login.pb.h"

#include "Packet.hpp"
#include "Serialization.hpp"

const int MAX_USERNAME_LENGTH = 128;

struct LoginPacket {
    uint32_t username_length;
    char username[MAX_USERNAME_LENGTH];
};

// template<>
// class Message<LoginPacket> {
// public:
//     static constexpr PacketType value = LOGIN_REQUEST_MSG;
// };




template<>
class tw::net::Serializer<LoginPacket> final {
public:
    static bool serialize(Serialization& buffer, LoginPacket& value) {
        buffer.serialize(&value.username_length);
        buffer.serialize(value.username, value.username_length);
        return true;
    }
};

// inline void to_json(json& j, const LoginPacket& value) {
//     j = json{
//         {"username_length", value.username_length},
//         {"username", std::string(value.username, value.username_length)}
//     };
// }

// inline void from_json(const json& j, LoginPacket& value) {
//     j.at("username_length").get_to(value.username_length);
//     j.at("username").get_to(value.username);
// }

struct LoginStatusPacket {
    bool is_okay;

    LoginStatusPacket() {
    }

    LoginStatusPacket(bool is_okay) :
        is_okay(is_okay)
    {
    }
};

template<>
class tw::net::Serializer<LoginStatusPacket> final {
public:
    static bool serialize(Serialization& buffer, LoginStatusPacket& value) {
        return buffer.serialize(&value.is_okay);
    }
};
