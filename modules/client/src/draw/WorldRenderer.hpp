#pragma once

#include <memory>

#include "Gpu.hpp"
#include "Window.hpp"
#include "Swapchain.hpp"
#include "draw/MeshBuffer.hpp"
#include "io/Files.hpp"
#include "io/ShaderManager.hpp"
#include "world/World.hpp"
#include "world/Camera.hpp"

#include "RenderGraphBuilder.hpp"

namespace tw::drw {

class WorldRenderer {
private:
    const lft::win::Window* m_window;
    Instance m_instance;
    Surface m_surface;
    Gpu m_gpu;
    Swapchain m_swapchain;

    const io::Files* m_files;
    io::ShaderManager m_shader_manager;

    VkFence m_wait_on_image_fence;
    Buffer m_camera_buffer;
    VkDescriptorSetLayout m_global_descriptor_set_layout;
    VkDescriptorSet m_global_descriptor_set;

    const World *m_world;

    MeshBuffer m_mesh_buffer;

    lft::rg::Builder m_rendergraph_builder;
    lft::rg::RenderGraph m_rendergraph;

    Camera m_camera;

    lft::rg::RenderGraph init_render_graph();

    void update_camera_buffer();

public:
    GET_MUT_REF(m_camera, camera);

    MeshBuffer* mesh_buffer() { return &m_mesh_buffer; }
    const MeshBuffer* mesh_buffer() const { return &m_mesh_buffer; }

    const World* world() const { return m_world; }

    const VkDescriptorSetLayout global_descriptor_set_layout() const {
        return m_global_descriptor_set_layout;
    }

    const VkDescriptorSet global_descriptor_set() const {
        return m_global_descriptor_set;
    }

    WorldRenderer(const std::string& name, 
            const lft::win::Window* window,
            const World* world,
            const io::Files* files);

    WorldRenderer(WorldRenderer&) = delete;

    WorldRenderer& operator=(WorldRenderer&) = delete;

    Mesh add_mesh(const MeshData& mesh_data) {
        return m_mesh_buffer.push_mesh(mesh_data);
    }

    void render();
};

}
