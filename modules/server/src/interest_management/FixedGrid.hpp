#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace tw::net::im {

/**
 * Bounded-world spatial grid backend.
 *
 * World area bounds are supplied at construction time so the same grid type
 * can be reused for differently-sized zones without recompiling.
 * CELL_SIZE and VIEW_RADIUS remain template parameters so kNeighborRadius
 * can be computed at compile time, keeping the hot-path loop bounds constant.
 *
 * Template parameters:
 *   CELL_SIZE:   size of each cell (meters). Recommended: CELL_SIZE == VIEW_RADIUS
 *                for maximum efficiency (no distance checks needed).
 *   VIEW_RADIUS: the subscription radius around a player (meters).
 *
 * Constructor parameters:
 *   min_x, max_x, min_z, max_z — world area bounds (meters).
 *
 * Complexity:
 *   begin_frame(): O(cols × rows) clearing
 *   insert():      O(1)
 *   query_neighbors(): O(kNeighborRadius²) cells × avg entities per cell
 *   query_area():      O(cells overlapping AABB) × avg entities per cell
 */
template<uint32_t CELL_SIZE, uint32_t VIEW_RADIUS>
class FixedGrid {
public:
    static constexpr uint32_t kCellSize      = CELL_SIZE;
    static constexpr uint32_t kViewRadius    = VIEW_RADIUS;
    static constexpr int32_t  kNeighborRadius =
        static_cast<int32_t>((VIEW_RADIUS + CELL_SIZE - 1) / CELL_SIZE);

    static_assert(kCellSize   > 0, "CELL_SIZE must be positive");
    static_assert(kViewRadius > 0, "VIEW_RADIUS must be positive");

private:
    int32_t  m_world_min_x, m_world_max_x;
    int32_t  m_world_min_z, m_world_max_z;
    uint32_t m_cols, m_rows;

    // Flat 2D vector of cell vectors indexed as [cz * m_cols + cx].
    // Outer vector is allocated once at construction; inner vectors retain
    // capacity across frames so no heap allocations occur in steady state.
    std::vector<std::vector<entt::entity>> m_cells;

    [[nodiscard]] inline std::pair<uint32_t, uint32_t> world_to_cell(
        float world_x, float world_z
    ) const {
        int32_t cx = static_cast<int32_t>((world_x - m_world_min_x) / CELL_SIZE);
        int32_t cz = static_cast<int32_t>((world_z - m_world_min_z) / CELL_SIZE);
        cx = std::max(0, std::min(cx, static_cast<int32_t>(m_cols) - 1));
        cz = std::max(0, std::min(cz, static_cast<int32_t>(m_rows) - 1));
        return { static_cast<uint32_t>(cx), static_cast<uint32_t>(cz) };
    }

    [[nodiscard]] inline uint32_t cell_index(uint32_t cx, uint32_t cz) const {
        return cz * m_cols + cx;
    }

public:
    FixedGrid(int32_t min_x, int32_t max_x, int32_t min_z, int32_t max_z)
        : m_world_min_x(min_x), m_world_max_x(max_x),
          m_world_min_z(min_z), m_world_max_z(max_z),
          m_cols((max_x - min_x + CELL_SIZE - 1) / CELL_SIZE),
          m_rows((max_z - min_z + CELL_SIZE - 1) / CELL_SIZE),
          m_cells(m_cols * m_rows)
    {}

    int32_t world_min_x() const { return m_world_min_x; }
    int32_t world_max_x() const { return m_world_max_x; }
    int32_t world_min_z() const { return m_world_min_z; }
    int32_t world_max_z() const { return m_world_max_z; }

    // Clears all cell vectors without releasing their capacity.
    void begin_frame() {
        for (auto& cell : m_cells) {
            cell.clear();
        }
    }

    void insert(entt::entity entity, glm::vec3 pos) {
        auto [cx, cz] = world_to_cell(pos.x, pos.z);
        m_cells[cell_index(cx, cz)].push_back(entity);
    }

    // Queries all entities in cells that overlap the given XZ AABB [min, max].
    // Entities in cells that partially extend beyond the exact boundary are included —
    // acceptable for zone-border queries where slight over-inclusion is harmless.
    void query_area(glm::vec2 min, glm::vec2 max, std::vector<entt::entity>& out) const {
        auto [cx_min, cz_min] = world_to_cell(min.x, min.y);
        auto [cx_max, cz_max] = world_to_cell(max.x, max.y);

        for (uint32_t cz = cz_min; cz <= cz_max; ++cz) {
            for (uint32_t cx = cx_min; cx <= cx_max; ++cx) {
                const auto& cell = m_cells[cell_index(cx, cz)];
                out.insert(out.end(), cell.begin(), cell.end());
            }
        }
    }

    void query_neighbors(glm::vec3 pos, std::vector<entt::entity>& out) {
        auto [center_x, center_z] = world_to_cell(pos.x, pos.z);

        for (int32_t dz = -kNeighborRadius; dz <= kNeighborRadius; ++dz) {
            for (int32_t dx = -kNeighborRadius; dx <= kNeighborRadius; ++dx) {
                int32_t cx = static_cast<int32_t>(center_x) + dx;
                int32_t cz = static_cast<int32_t>(center_z) + dz;

                if (cx < 0 || cx >= static_cast<int32_t>(m_cols) ||
                    cz < 0 || cz >= static_cast<int32_t>(m_rows)) {
                    continue;
                }

                const auto& cell = m_cells[cell_index(cx, cz)];
                out.insert(out.end(), cell.begin(), cell.end());
            }
        }
    }
};

} // namespace tw::net::im
