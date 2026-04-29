#pragma once

#include "common.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace tw::net::im {

using InterestId = uint32_t;

// Axis-aligned bounding box in the XZ plane (vec2.x = world X, vec2.y = world Z).
struct AreaBounds {
    glm::vec2 min;
    glm::vec2 max;

    bool is_overlapping(glm::vec2 point) {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
    }
};

// Unified interest subscription: tracks entities near a world-space entity position
// (entity interest) or within a quad area (area interest).
//
// The identity of the caller — whether it is a session, a neighbouring zone, or
// anything else — is the caller's concern.  InterestSystem only sees an InterestId.
//
// No heap allocations in steady state after the first frame.
class Interest {
    std::variant<entt::entity, AreaBounds> m_source;
    std::vector<entt::entity> m_interest;      // sorted, persistent
    std::vector<entt::entity> m_new_interest;  // scratch, reused each frame
    std::vector<entt::entity> m_spawn;
    std::vector<entt::entity> m_despawn;

    void reserve() {
        m_interest.reserve(512);
        m_new_interest.reserve(512);
        m_spawn.reserve(256);
        m_despawn.reserve(256);
    }

public:
    GET_REF(m_interest, interest);
    GET_MUT_REF(m_spawn,   spawn);
    GET_MUT_REF(m_despawn, despawn);

    bool is_entity_interest() const { return std::holds_alternative<entt::entity>(m_source); }
    bool is_area_interest()   const { return std::holds_alternative<AreaBounds>(m_source); }

    entt::entity      entity() const { return std::get<entt::entity>(m_source); }
    const AreaBounds& bounds() const { return std::get<AreaBounds>(m_source); }

    std::vector<entt::entity>& new_interest() { return m_new_interest; }

    void swap_interest(std::vector<entt::entity>& other) {
        m_interest.swap(other);
    }

    bool is_interested_in_entity(entt::entity e) const {
        return std::binary_search(m_interest.begin(), m_interest.end(), e);
    }

    explicit Interest(entt::entity entity) : m_source(entity) { reserve(); }
    explicit Interest(AreaBounds   bounds) : m_source(bounds)  { reserve(); }
};

} // namespace tw::net::im
