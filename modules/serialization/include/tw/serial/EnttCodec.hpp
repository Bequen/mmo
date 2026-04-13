#pragma once

/**
 * Codec specialisation for entt::entity.
 *
 * Wire format: uint32_t  (the raw entity storage value, 4 bytes).
 *
 * EnTT entities are 32-bit identifiers internally (version bits + index bits).
 * We transmit the raw value and let the receiver reconstruct via
 * static_cast<entt::entity>(id).
 */

#include "Codec.hpp"

#include <entt/entt.hpp>

namespace tw::serial {

template<>
struct Codec<entt::entity> {
    static void encode(BinaryWriter& w, const entt::entity& e) noexcept {
        auto raw = static_cast<uint32_t>(e);
        w.write(raw);
    }
    static entt::entity decode(BinaryReader& r) noexcept {
        return static_cast<entt::entity>(r.read<uint32_t>());
    }
};

} // namespace tw::serial
