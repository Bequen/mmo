#pragma once

#include "Instance.hpp"
#include "draw/WorldRenderer.hpp"
#include "io/ShaderManager.hpp"
#include "shaders/PipelineBuilder.h"
#include "world/ChunkData.hpp"
#include <spdlog/spdlog.h>

namespace tw::drw {

class TerrainRenderPass {
private:
    using Self = TerrainRenderPass;

    const WorldRenderer *m_world_renderer;
    io::ShaderManager *m_shader_manager;

    VkPipelineLayout m_pipeline_layout;
    Pipeline m_pipeline;

    lft::rg::RenderTaskBuilder m_task;

    Mesh m_chunk_mesh;

    Mesh& get_chunk_mesh(glm::ivec2 position) {
        return m_chunk_mesh;
    }

    lft::rg::RenderTaskBuilder create_render_task() {
        return lft::rg::render_task<Self>(
                "terrain", this,
                [](const lft::rg::TaskBuildInfo& info,
                    Self* context) {
                    auto vertex_shader = context->m_shader_manager->load_shader("Opaque2.vert.spirv");
                    auto fragment_shader = context->m_shader_manager->load_shader("Opaque2.frag.spirv");

    				context->m_pipeline_layout = PipelineLayoutBuilder()
    					.input_set(0, context->m_world_renderer->global_descriptor_set_layout())
                        .push_constant_range(0, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT)
    					.build(info.gpu());

                    std::println("Viewport: {} {}", info.viewport().width, info.viewport().height);

                    context->m_pipeline = PipelineBuilder(info.gpu(), info.viewport(),
                            context->m_pipeline_layout, info.renderpass(),
                            1,
                            &vertex_shader, &fragment_shader)
                        .set_vertex_input_info({
                                    {
                                            .binding = 0,
                                            .stride = sizeof(glm::vec3),
                                            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                                    },
                                }, {
                                    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
                                    { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) * 1000 },
                                })
                        .build();
                },
                [](const lft::rg::TaskRecordInfo& info,
                    Self* context) {
                    info.recording()
                        .bind_vertex_buffers({context->m_world_renderer->mesh_buffer()->buffer()}, {0});

                    auto binded_pipeline = info.recording()
                        .bind_graphics_pipeline(context->m_pipeline)
                        .bind_descriptor_set(0, context->m_world_renderer->global_descriptor_set());

                    auto viewport = info.viewport();
                    VkRect2D rect = {
                        .offset = {
                            .x = 0,
                            .y = 0
                        }, .extent = {
                            .width = (uint32_t)viewport.width,
                            .height = (uint32_t)std::abs(viewport.height)
                        }
                    };
                    vkCmdSetViewport(info.recording().cmdbuf(), 0, 1, &viewport);
                    vkCmdSetScissor(info.recording().cmdbuf(), 0, 1, &rect);

                    glm::vec3 center = context->m_world_renderer->camera().position();

                    uint32_t render_distance = 4;

                    glm::vec3 from = glm::floor(center - glm::vec3(render_distance));
                    glm::vec3 to = glm::ceil(center + glm::vec3(render_distance));

                    from = glm::vec3(-4, 0, -4);
                    to = glm::vec3(4, 0, 4);

                    for(int32_t x = from.x; x < to.x; x++) {
                        for(int32_t z = from.z; z < to.z; z++) {
                            auto& mesh = context->get_chunk_mesh(glm::ivec2(x, z));

                            glm::mat4 ts = glm::translate(glm::mat4(1.0f), glm::vec3(x * CHUNK_WIDTH, 0, z * CHUNK_WIDTH));
                            binded_pipeline.push_constants(VK_SHADER_STAGE_VERTEX_BIT, 0,
                                    sizeof(glm::mat4), &ts);

                            info.recording().draw(mesh.num_vertices(), 1, mesh.vertex_offset(), 0);
                        }
                    }
                }
        )
            .set_depth_output("gbuf_depth", VK_FORMAT_D32_SFLOAT_S8_UINT)
            .set_output_to_final();
    }

public:
    TerrainRenderPass(
            WorldRenderer* world_renderer,
            io::ShaderManager* shader_manager
    ) :
        m_world_renderer(world_renderer),
        m_shader_manager(shader_manager),
        m_task(create_render_task()),
        m_pipeline(VK_NULL_HANDLE, VK_NULL_HANDLE),
        m_chunk_mesh(world_renderer->mesh_buffer()->push_mesh(MeshData::plane(glm::vec2(16.0f))))
    {
    }

    lft::rg::RenderTaskBuilder& get_render_task() {
        return m_task;
    }
};

}
