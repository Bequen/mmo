#pragma once

#include <array>
#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace tw::net::im {

/**
 * Bounded-world spatial grid backend.
 *
 * Uses a flat 2D array of cells indexed by XZ position. All cells are contiguous
 * in memory for optimal cache performance. Does not perform any heap allocations
 * in the hot path (query_neighbors) — all vectors are pre-allocated and reused.
 *
 * Neighborhood is always a square of (NEIGHBOR_RADIUS × NEIGHBOR_RADIUS) cells,
 * where NEIGHBOR_RADIUS = ceil(VIEW_RADIUS / CELL_SIZE).
 *
 * Template parameters:
 *   WORLD_MIN_X, WORLD_MAX_X: world bounds in X (meters)
 *   WORLD_MIN_Z, WORLD_MAX_Z: world bounds in Z (meters)
 *   CELL_SIZE: size of each cell (meters). Recommended: CELL_SIZE == VIEW_RADIUS
 *              for maximum efficiency (no distance checks needed).
 *   VIEW_RADIUS: the subscription radius around a player (meters).
 *
 * Complexity:
 *   begin_frame(): O(COLS * ROWS) clearing
 *   insert(): O(1)
 *   query_neighbors(): O(NEIGHBOR_RADIUS²) cells, O(avg entities per cell)
 */
template<
    int32_t  WORLD_MIN_X,
    int32_t  WORLD_MAX_X,
    int32_t  WORLD_MIN_Z,
    int32_t  WORLD_MAX_Z,
    uint32_t CELL_SIZE,
    uint32_t VIEW_RADIUS
>
class FixedGrid {
public:
    static constexpr int32_t  kWorldMinX = WORLD_MIN_X;
    static constexpr int32_t  kWorldMaxX = WORLD_MAX_X;
    static constexpr int32_t  kWorldMinZ = WORLD_MIN_Z;
    static constexpr int32_t  kWorldMaxZ = WORLD_MAX_Z;
    static constexpr uint32_t kCellSize  = CELL_SIZE;
    static constexpr uint32_t kViewRadius = VIEW_RADIUS;

    // Grid dimensions
    static constexpr uint32_t kCols =
        (WORLD_MAX_X - WORLD_MIN_X + CELL_SIZE - 1) / CELL_SIZE;
    static constexpr uint32_t kRows =
        (WORLD_MAX_Z - WORLD_MIN_Z + CELL_SIZE - 1) / CELL_SIZE;
    static constexpr uint32_t kTotalCells = kCols * kRows;

    // Neighborhood radius in cells
    static constexpr int32_t kNeighborRadius =
        static_cast<int32_t>((VIEW_RADIUS + CELL_SIZE - 1) / CELL_SIZE);

    static_assert(kTotalCells > 0, "Grid must have at least one cell");
    static_assert(kTotalCells <= 65536, "Grid is too large; use SpatialHashGrid instead");
    static_assert(kCellSize > 0, "CELL_SIZE must be positive");
    static_assert(kViewRadius > 0, "VIEW_RADIUS must be positive");

private:
    // Flat 2D array of cell vectors. All contiguous, zero indirection.
    std::array<std::vector<entt::entity>, kTotalCells> m_cells;

    // Helper: convert world XZ coordinates to cell grid coordinates (clamped).
    [[nodiscard]] inline std::pair<uint32_t, uint32_t> world_to_cell(
        float world_x, float world_z
    ) const {
        int32_t cx = static_cast<int32_t>((world_x - WORLD_MIN_X) / CELL_SIZE);
        int32_t cz = static_cast<int32_t>((world_z - WORLD_MIN_Z) / CELL_SIZE);

        // Clamp to valid range [0, COLS-1] and [0, ROWS-1]
        cx = std::max(0, std::min(cx, static_cast<int32_t>(kCols) - 1));
        cz = std::max(0, std::min(cz, static_cast<int32_t>(kRows) - 1));

        return {static_cast<uint32_t>(cx), static_cast<uint32_t>(cz)};
    }

    // Helper: convert cell coordinates to flat array index.
    [[nodiscard]] inline uint32_t cell_index(uint32_t cx, uint32_t cz) const {
        return cz * kCols + cx;
    }

public:
    FixedGrid() = default;

    // Clears all cell vectors. Does not deallocate memory — capacity is retained
    // for the next frame. O(kTotalCells) amortized O(1) per cell.
    void begin_frame() {
        for (auto& cell : m_cells) {
            cell.clear();
        }
    }

    // Inserts an entity at the given world position into the appropriate cell.
    void insert(entt::entity entity, glm::vec3 pos) {
        auto [cx, cz] = world_to_cell(pos.x, pos.z);
        m_cells[cell_index(cx, cz)].push_back(entity);
    }

    // Queries the neighborhood around the given position.
    // Appends all entities in the neighborhood cells into out_entities.
    void query_neighbors(glm::vec3 pos, std::vector<entt::entity>& out_entities) {
        auto [center_x, center_z] = world_to_cell(pos.x, pos.z);

        // Iterate the square neighborhood around (center_x, center_z)
        for (int32_t dz = -kNeighborRadius; dz <= kNeighborRadius; ++dz) {
            for (int32_t dx = -kNeighborRadius; dx <= kNeighborRadius; ++dx) {
                int32_t cx = static_cast<int32_t>(center_x) + dx;
                int32_t cz = static_cast<int32_t>(center_z) + dz;

                // Skip cells outside grid bounds
                if (cx < 0 || cx >= static_cast<int32_t>(kCols) ||
                    cz < 0 || cz >= static_cast<int32_t>(kRows)) {
                    continue;
                }

                const auto& cell = m_cells[cell_index(cx, cz)];
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
