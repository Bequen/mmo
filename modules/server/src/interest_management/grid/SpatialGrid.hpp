#pragma once

// DEPRECATED: This file is kept for reference only.
// The interest management system has been redesigned with a new architecture:
//
// ✓ SpatialBackend.hpp        — C++20 concept defining the spatial backend interface
// ✓ FixedGrid.hpp             — Bounded world backend (maximum speed, zero allocations)
// ✓ SpatialHashGrid.hpp       — Unbounded world backend (flexible, ~15% slower)
// ✓ ClientState.hpp           — Tracks per-client interest state
// ✓ InterestSystem.hpp        — Templated system accepting any SpatialBackend
// ✓ StateReplicator.hpp       — Updated to use ClientState* directly
//
// The new design delivers:
// - O(E) grid rebuild + O(P × K) interest queries (was O(P × E) brute force)
// - Zero heap allocations in steady state (all vectors reused)
// - Independently parallelizable per-client updates (no shared state)
// - Support for both bounded (FixedGrid) and unbounded (SpatialHashGrid) worlds
//
// To migrate:
// 1. Include "interest_management/FixedGrid.hpp" or "interest_management/SpatialHashGrid.hpp"
// 2. Define your backend: using Grid = im::FixedGrid<-5000, 5000, -5000, 5000, 500, 500>;
// 3. Create InterestSystem<Grid> and StateReplicator<Grid> instances
// 4. See WorldServer.hpp for an example configuration

#include <vector>
#include <entt/entt.hpp>

namespace tw::net::im::grid {

// Legacy cell class — kept for reference
class SpatialGridCell {
    std::vector<entt::entity> m_entities;

public:
    SpatialGridCell() = default;

    void append_entity(entt::entity entity) {
        m_entities.push_back(entity);
    }

    void move_entity(entt::entity entity, SpatialGridCell& newCell) {
        auto it = std::find(m_entities.begin(), m_entities.end(), entity);
        if (it != m_entities.end()) {
            m_entities.erase(it);
            newCell.append_entity(entity);
        }
    }
};

// Legacy grid class — REMOVED, use FixedGrid or SpatialHashGrid instead
// class SpatialGrid { ... };

}
