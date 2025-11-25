#include "MeshBuffer.hpp"

namespace tw::drw {

void MeshBuffer::resize_buffer(size_t num_vertices) {
    BufferCreateInfo buffer_info = {
        .size = sizeof(glm::vec3) * 2 * num_vertices,
        .usage = VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
        .isExclusive = true,
    };

    MemoryAllocationInfo memory_info = {
        .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    m_gpu->memory()->create_buffer(&buffer_info, &memory_info, &m_vertex_buffer);
    m_max_vertex_count = num_vertices;
}

MeshBuffer::MeshBuffer(const Gpu* gpu) :
m_gpu(gpu),
m_stream(gpu, 1000 * sizeof(glm::vec3) * 1),
m_num_vertices(0) {
    resize_buffer(1000);
}

Mesh MeshBuffer::push_mesh(const MeshData& mesh_data) {
    m_stream.write(&m_vertex_buffer, (void*)mesh_data.positions().data(), m_num_vertices * sizeof(glm::vec3), mesh_data.positions().size() * sizeof(glm::vec3));

    m_stream.write(&m_vertex_buffer, (void*)mesh_data.normals().data(), (m_num_vertices + 1000) * sizeof(glm::vec3), mesh_data.normals().size() * sizeof(glm::vec3));

    m_stream.flush();
    
    Mesh result(m_num_vertices, mesh_data.positions().size());

    m_num_vertices += mesh_data.positions().size();

    return result;
}

}
