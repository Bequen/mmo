#include "GuiRenderer.hpp"

#include "ImGuiRenderPass.hpp"

#include "SDLWindow.h"
#include "Window.hpp"

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

lft::rg::RenderGraph GuiRenderer::init_render_graph() {
    auto imgui_render_pass = new tw::drw::gui::ImGuiRenderPass(&m_gpu, (const lft::win::SDLWindow*)m_window);

    m_render_graph_builder.add_task(imgui_render_pass->get_render_task().build());

    auto render_graph = m_render_graph_builder.build();
    return std::move(render_graph);
}

GuiRenderer::GuiRenderer(const lft::win::Window* window) :
    m_window(window),
    m_instance(create_instance("ServerGui", window)),
    m_surface(create_surface((lft::win::SDLWindow*)window, &m_instance)),
    m_gpu(&m_instance, &m_surface),
    m_swapchain(&m_gpu, m_window->get_size(), &m_surface),
    m_render_graph_builder(&m_gpu, ImageChain::from_swapchain(m_swapchain), "swapchain"),
    m_render_graph(init_render_graph())
{ }
