#pragma once

#include "Serializers.hpp"

// #define PACKET(name) struct #name {




// template<>
// class tw::net::Serializer<const PacketType> final {
// public:
//     static bool serialize(tw::net::Serialization& buffer, const PacketType& value) {
//         uint32_t v = value;
//         return buffer.serialize((uint32_t*)&v);
//     }
// };


// template<>
// class tw::net::Serializer<PacketType> final {
// public:
//     static bool serialize(tw::net::Serialization& buffer, PacketType& value) {
//         return buffer.serialize((uint32_t*)&value);
//     }
// };
