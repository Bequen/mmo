#include "WorldRenderer.hpp"

#include <memory>
#include <vector>
#include <string>
#include <print>

#include "imgui.h"

#include "shaders/ShaderInputSet.h"
#include "shaders/ShaderInputSetLayoutBuilder.hpp"
#include "SDLWindow.h"
#include "Swapchain.hpp"
#include "RenderGraphBuilder.hpp"

#include "draw/RenderPasses/CharacterRenderPass.hpp"
#include "ImGuiRenderPass.hpp"
#include "draw/RenderPasses/TerrainRenderPass.hpp"
#include "world/Camera.hpp"
#include "world/World.hpp"

namespace tw::drw {

void lft_dbg_callback(lft::dbg::LogMessageSeverity severity,
                      lft::dbg::LogMessageType type,
                      const char *__restrict format,
                      va_list args) {
    const char* titles[3] = {
            "\033[0;34m[info]:",
            "\033[0;33m[warn]:",
            "\033[0;31m[fail]:"
    };
    fwrite(titles[severity], 14, 1, stdout);

    if(args != nullptr) {
        vfprintf(stdout, format, args);
    } else {
        fwrite(format, 1, strlen(format), stdout);
    }

    fwrite("\033[0m", 4, 1, stdout);

    printf("\n");
}


Instance create_instance(const std::string& name, const lft::win::Window* window) {
    std::vector<std::string> required_extensions = window->get_required_extensions();
    // required_extensions.push_back(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);

    std::vector<std::string> required_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    /**
     * Instance initializes a connection with Vulkan driver
     */
    return Instance(
			name, name,
			required_extensions,
			required_layers,
			lft_dbg_callback);
}

Surface create_surface(lft::win::SDLWindow* window, const Instance* instance) {
    return window->create_surface(instance);
}

lft::rg::RenderGraph WorldRenderer::init_render_graph() {
    auto m_imgui_rp = new gui::ImGuiRenderPass(&m_gpu, (const lft::win::SDLWindow*)m_window);
    auto m_character_rp = new CharacterRenderPass(this, &m_shader_manager);
    auto m_terrain_rp = new TerrainRenderPass(this, &m_shader_manager);

    m_rendergraph_builder.add_task(m_imgui_rp->get_render_task().build());
    m_rendergraph_builder.add_task(m_character_rp->get_render_task().build());
    m_rendergraph_builder.add_task(m_terrain_rp->get_render_task().build());

    return m_rendergraph_builder.build();
}

VkDescriptorSetLayout create_global_descriptor_set_layout(const Gpu* gpu) {
    return ShaderInputSetLayoutBuilder()
            .uniform_buffer(0)
            .build(gpu);
}

VkDescriptorSet create_global_descriptor_set(
        const Gpu* gpu,
        VkDescriptorSetLayout descriptor_set_layout,
        Buffer camera_buffer
) {
    return ShaderInputSetBuilder()
            .buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, camera_buffer, 0, sizeof(glm::mat4) * 2 + sizeof(glm::vec4))
            .build(gpu, descriptor_set_layout);
}

Buffer create_camera_buffer(const Gpu *gpu) {
	MemoryAllocationInfo allocInfo = {
		.usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
		.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};

    BufferCreateInfo bufferInfo = {
            .size = sizeof(CameraData),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true
    };

    Buffer buffer;
    gpu->memory()->create_buffer(&bufferInfo, &allocInfo, &buffer);

    return buffer;
}

WorldRenderer::WorldRenderer(
        const std::string& name,
        const lft::win::Window* window,
        const World* world,
        const io::Files* files
) :
    m_window(window),
    m_surface(create_surface((lft::win::SDLWindow*)(m_window), &m_instance)),
    m_instance(create_instance(name, m_window)),
    m_gpu(&m_instance, &m_surface),
    m_swapchain(&m_gpu, m_window->get_size(), &m_surface),

    m_files(files),
    m_shader_manager(&m_gpu, m_files),

    m_wait_on_image_fence(m_gpu.create_fence(false)),
    m_camera_buffer(create_camera_buffer(&m_gpu)),
    m_global_descriptor_set_layout(create_global_descriptor_set_layout(&m_gpu)),
    m_global_descriptor_set(create_global_descriptor_set(&m_gpu, m_global_descriptor_set_layout, m_camera_buffer)),

    m_world(world),
    m_mesh_buffer(&m_gpu),

    m_rendergraph_builder(&m_gpu, ImageChain::from_swapchain(m_swapchain), "swapchain"),
    m_rendergraph(init_render_graph()),
    m_camera(720.0f / 480.0f)
{
}

void WorldRenderer::update_camera_buffer() {
    void *pData = nullptr;
    m_gpu.memory()->map(m_camera_buffer.allocation, &pData);

    memcpy(pData, m_camera.data(), sizeof(CameraData));

    m_gpu.memory()->unmap(m_camera_buffer.allocation);
}

void WorldRenderer::render() {
    if(!m_window->has_size(m_last_window_size)) {
        vkDeviceWaitIdle(m_gpu.dev());
        m_swapchain.resize(m_window->get_size());
        m_rendergraph_builder.set_image_chain(ImageChain::from_swapchain(m_swapchain));
        m_rendergraph = m_rendergraph_builder.build();
        m_last_window_size = m_window->get_size();
    }

    uint32_t imageIdx = 0;
    auto result = m_swapchain.get_next_image_idx(
            VK_NULL_HANDLE, m_wait_on_image_fence, &imageIdx);


    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        std::println("Rebuilding");
        m_rendergraph_builder.build();
    }

    update_camera_buffer();

    m_rendergraph.run(imageIdx, VK_NULL_HANDLE, m_wait_on_image_fence);

    m_swapchain.present({ m_rendergraph.buffer(0).final_signal(imageIdx) }, imageIdx);
}

}
