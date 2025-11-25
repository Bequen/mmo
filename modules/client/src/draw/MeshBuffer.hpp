#pragma once

#include "Gpu.hpp"
#include "resources/BufferBusWriter.h"

#include "Mesh.hpp"
#include "MeshData.hpp"

namespace tw::drw {

class MeshBuffer {
private:
    const Gpu* m_gpu;

    Buffer m_vertex_buffer;
    uint32_t m_max_vertex_count;
    uint32_t m_num_vertices;

    // streams data from CPU to the GPU
    BufferBusWriter m_stream;

    void resize_buffer(size_t num_vertices);

public:
    inline const Buffer& buffer() const { return m_vertex_buffer; }

    MeshBuffer(const Gpu* gpu);

    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    
    Mesh push_mesh(const MeshData& mesh_data);
};

}
