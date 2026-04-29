#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace tw::net::im {

/**
 * Unbounded-world spatial hash grid backend.
 *
 * Uses an open hash map with cells indexed by packed (cell_x, cell_z) coordinates.
 * Supports any world coordinate range without pre-allocation.
 *
 * Neighborhood is always a square of (NEIGHBOR_RADIUS × NEIGHBOR_RADIUS) cells,
 * where NEIGHBOR_RADIUS = ceil(VIEW_RADIUS / CELL_SIZE).
 *
 * Template parameters:
 *   CELL_SIZE: size of each cell (meters). Recommended: CELL_SIZE == VIEW_RADIUS.
 *   VIEW_RADIUS: the subscription radius around a player (meters).
 *
 * Complexity:
 *   begin_frame(): O(num_cells_with_entities)
 *   insert(): O(1) amortized
 *   query_neighbors(): O(NEIGHBOR_RADIUS²) hash lookups, O(avg entities per cell)
 *
 * ~15-20% slower than FixedGrid due to hash overhead per cell lookup,
 * but more flexible for unbounded or procedural worlds.
 */
template<uint32_t CELL_SIZE, uint32_t VIEW_RADIUS>
class SpatialHashGrid {
public:
    static constexpr uint32_t kCellSize    = CELL_SIZE;
    static constexpr uint32_t kViewRadius  = VIEW_RADIUS;
    static constexpr int32_t  kNeighborRadius =
        static_cast<int32_t>((VIEW_RADIUS + CELL_SIZE - 1) / CELL_SIZE);

    static_assert(kCellSize > 0, "CELL_SIZE must be positive");
    static_assert(kViewRadius > 0, "VIEW_RADIUS must be positive");

private:
    // Hash map: key is packed (cell_x << 32) | cell_z (both as uint32_t from int32_t)
    // value is a vector of entities in that cell.
    std::unordered_map<uint64_t, std::vector<entt::entity>> m_cells;

    // Helper: pack cell coordinates into a single uint64_t key.
    // Negative int32_t coordinates are safely encoded via two's complement
    // cast to uint32_t.
    [[nodiscard]] static inline uint64_t make_key(int32_t cx, int32_t cz) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
               static_cast<uint32_t>(cz);
    }

    // Helper: convert world XZ coordinates to cell grid coordinates.
    [[nodiscard]] static inline std::pair<int32_t, int32_t> world_to_cell(
        float world_x, float world_z
    ) {
        // Floor division to handle negative coordinates correctly.
        // For negative coordinates, std::floor ensures we round down, not toward zero.
        int32_t cx = static_cast<int32_t>(std::floor(world_x / CELL_SIZE));
        int32_t cz = static_cast<int32_t>(std::floor(world_z / CELL_SIZE));
        return {cx, cz};
    }

public:
    SpatialHashGrid() = default;

    // Clears all cell vectors. Does not erase map entries or deallocate memory —
    // preserves bucket structure and vector capacity for reuse.
    // O(num_cells_with_entities) amortized O(1) per cell.
    void begin_frame() {
        for (auto& [key, cell] : m_cells) {
            cell.clear();
        }
    }

    // Inserts an entity at the given world position into the appropriate cell.
    void insert(entt::entity entity, glm::vec3 pos) {
        auto [cx, cz] = world_to_cell(pos.x, pos.z);
        m_cells[make_key(cx, cz)].push_back(entity);
    }

    // Queries all entities in cells that overlap the given XZ AABB [min, max].
    void query_area(glm::vec2 min, glm::vec2 max, std::vector<entt::entity>& out_entities) {
        auto [cx_min, cz_min] = world_to_cell(min.x, min.y);
        auto [cx_max, cz_max] = world_to_cell(max.x, max.y);

        for (int32_t cz = cz_min; cz <= cz_max; ++cz) {
            for (int32_t cx = cx_min; cx <= cx_max; ++cx) {
                auto it = m_cells.find(make_key(cx, cz));
                if (it == m_cells.end()) continue;
                const auto& cell = it->second;
                out_entities.insert(out_entities.end(), cell.begin(), cell.end());
            }
        }
    }

    // Queries the neighborhood around the given position.
    // Appends all entities in the neighborhood cells into out_entities.
    void query_neighbors(glm::vec3 pos, std::vector<entt::entity>& out_entities) {
        auto [center_x, center_z] = world_to_cell(pos.x, pos.z);

        // Iterate the square neighborhood around (center_x, center_z)
        for (int32_t dz = -kNeighborRadius; dz <= kNeighborRadius; ++dz) {
            for (int32_t dx = -kNeighborRadius; dx <= kNeighborRadius; ++dx) {
                int32_t cx = center_x + dx;
                int32_t cz = center_z + dz;

                uint64_t key = make_key(cx, cz);
                auto it = m_cells.find(key);
                if (it == m_cells.end()) {
                    // No entities in this cell, skip
                    continue;
                }

                const auto& cell = it->second;
                out_entities.insert(
                    out_entities.end(),
                    cell.begin(),
                    cell.end()
                );
            }
        }
    }
};

} // namespace tw::net::im
