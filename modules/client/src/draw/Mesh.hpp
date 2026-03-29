#pragma once

#include <cstdint>

namespace tw::drw {

/**
 * Stores location data about Mesh on the GPU.
 */
class Mesh {
    uint32_t m_vertex_offset;
    uint32_t m_num_vertices;

public:
    inline uint32_t num_vertices() const {
        return m_num_vertices;
    }

    inline uint32_t vertex_offset() const {
        return m_vertex_offset;
    }

    Mesh(uint32_t vertex_offset, uint32_t num_vertices) :
        m_vertex_offset{vertex_offset},
        m_num_vertices{num_vertices} {
    }
};

}
