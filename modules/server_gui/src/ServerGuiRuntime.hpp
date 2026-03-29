#include "GuiRenderer.hpp"
#include "SDLWindow.h"
#include "Window.hpp"
#include "runtime/LockStep.hpp"

#include <memory>

class ServerGuiRuntime {
    std::unique_ptr<lft::win::Window> m_window;

    std::unique_ptr<GuiRenderer> m_renderer;

    bool m_is_running;

    std::unique_ptr<lft::win::Window> create_window(const std::string& name, VkExtent2D extent) {
        return std::make_unique<lft::win::SDLWindow>(name, (VkRect2D){
                0, 0,
                extent.width, extent.height
        });
    }

public:
    ServerGuiRuntime() :
    m_window(create_window("ServerGui", {1024, 1024})),
    m_is_running(true) {

    }

    void run() {
        tw::LockStep lock_step(20);

        while(m_is_running) {
            if(lock_step.wait_for_next_step()) {
                continue;
            }

            // ImGui_ImplVulkan_NewFrame();
            // ImGui_ImplSDL2_NewFrame();
            // ImGui::NewFrame();

            // ImGui::ShowDemoWindow();
        }
    }
};
