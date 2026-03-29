#pragma once

#include "RenderPass.hpp"
#include "SDLWindow.h"
#include "io/file.hpp"

namespace tw::drw::gui {

class ImGuiRenderPass {
    const Gpu* m_gpu;
    const lft::win::SDLWindow* m_window;

    bool is_initialized;

    lft::rg::RenderTaskBuilder m_task;

    lft::rg::RenderTaskBuilder create_render_task();

public:
    ImGuiRenderPass(const Gpu* gpu, const lft::win::SDLWindow* window) :
        m_gpu(gpu),
        m_window(window),
        is_initialized(false),
        m_task(create_render_task()) {
    }

    lft::rg::RenderTaskBuilder& get_render_task() {
        return m_task;
    }
};

}
