#pragma once

#include "ChunkData.hpp"
#include "draw/MeshData.hpp"



namespace tw {

constexpr std::vector<uint8_t> CHUNK_VERTICES() {

}

class ChunkMeshData {
    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec3> m_normal;

    std::vector<uint16_t> m_indices;

public:
    ChunkMeshData(const ChunkData& chunk) {
        auto mesh_data = drw::MeshData::plane(glm::vec2(8.0f, 8.0f));
    }
};

}
