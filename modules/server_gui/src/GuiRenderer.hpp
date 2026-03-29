#pragma once

#include "Gpu.hpp"
#include "RenderGraph.hpp"
#include "RenderGraphBuilder.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"

class GuiRenderer {
private:
    const lft::win::Window* m_window;

    Instance m_instance;

    Surface m_surface;

    Gpu m_gpu;

    Swapchain m_swapchain;

    lft::rg::Builder m_render_graph_builder;
    lft::rg::RenderGraph m_render_graph;

    lft::rg::RenderGraph init_render_graph();

public:
    GuiRenderer(const lft::win::Window* window);

    void render() {
        // uint32_t imageIdx = 0;
        // auto result = m_swapchain.get_next_image_idx(
        //         VK_NULL_HANDLE, m_wait_on_image_fence, &imageIdx);

        // m_rendergraph.run(imageIdx, VK_NULL_HANDLE, m_wait_on_image_fence);

        // m_swapchain.present({ m_rendergraph.buffer(0).final_signal(imageIdx) }, imageIdx);
    }
};
