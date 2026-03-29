#include "runtime.hpp"

#include "Address.hpp"
#include "SDLWindow.h"
#include "debug/tools/NetworkStatsGui.hpp"
#include "debug/tools/PacketBacklogGui.hpp"
#include "entt/entity/fwd.hpp"
#include "io/InputState.hpp"
#include "debug/tools/EntityManagerGui.hpp"
#include "debug/tools/PerformanceStatsGui.hpp"
#include "draw/MeshData.hpp"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl2.h"

#include "implot.h"
#include "implot_internal.h"
#include "world/Transform.hpp"

#include <SDL_events.h>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <memory>

namespace tw {

std::unique_ptr<lft::win::Window> create_window(const std::string& name, VkExtent2D extent) {
    return std::make_unique<lft::win::SDLWindow>(name, (VkRect2D){
            0, 0,
            extent.width, extent.height
    });
}

int get_port_from_args(int argc, char** argv) {
    try {
        if(argc > 1) {
            return atoi(argv[1]);
        } else {
            return 8080;
        }
    } catch(std::exception& e) {
        std::println("Could not parse port from arguments, using default.");
        return 8080;
    }
}

Runtime::Runtime(int argc, char** argv) :
    m_files(argv, argc),
    m_window(create_window("towards", {1920, 1200})),
    m_world{},
    m_physics_world(&m_world),
    m_world_renderer("towards", m_window.get(), &m_world, &m_files),
    m_input_manager(m_window.get()),
    m_world_controller(&m_input_manager, &m_world, &m_physics_world, &m_world_renderer, { "127.0.0.1", get_port_from_args(argc, argv) }),
    m_lockstep(60)
{
}

// bool Runtime::world_state_packet_handler(uint32_t* p_frame_idx, WorldSnapshotMessage* mesg) {
//     uint32_t frame_idx = *p_frame_idx;

//     if(mesg->frame_idx < frame_idx) {
//         return false;
//     }


//     for(int i = 0; i < mesg->player_states.size(); i++) {
//         if(!m_players.contains(mesg->player_states[i].id)) {
//             auto mesh = m_world_renderer.add_mesh(drw::MeshData::cube(glm::vec3(1.0f)));
//             const auto entity = m_world.registry().create();
//             m_players.insert({mesg->player_states[i].id, entity});

//             m_world.registry().emplace<Transform>(entity, Transform(mesg->player_states[i].position));
//             m_world.registry().emplace<PlayerInfoComponent>(entity,
//                     PlayerInfoComponent(
//                         mesg->player_states[i].id,
//                         mesg->player_states[i].name));
//             m_world.registry().emplace<drw::Mesh>(entity, mesh);

//         } else {
//             auto entity = (entt::entity)mesg->player_states[i].id;
//             auto entity_ts = m_world.registry()
//                 .try_get<Transform>(entity);

//             if(entity_ts) {
//                 entity_ts->transform = glm::translate(glm::mat4(1.0f), mesg->player_states[i].position);
//             }
//         }
//     }

//     return true;
// }

void Runtime::run() {
    dbg::tools::EntityManagerGui entity_manager(&m_world);
    dbg::tools::PerformanceStatsGui perf_stats(m_lockstep);
    dbg::tools::PacketBacklogGui packet_backlog;
    dbg::tools::NetworkStatsGui network_stats;

    ImPlot::CreateContext();
    m_is_running = true;

    while(is_running()) {
        if(m_lockstep.wait_for_next_step()) {
            continue;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ImGui::DockSpaceOverViewport();

        entity_manager.draw();

        ImGui::Begin("History");

        if(ImGui::BeginTable("historyTable", 2)) {
            // for(auto key : m_world_controller.position_history().keys()) {
            //     ImGui::TableNextRow();
            //
            //     // auto v = *m_world_controller.player_history().get(key).value();
            //     // auto input = std::format("{} {} {}", v.x, v.y, v.z);
            //     auto p = *m_world_controller.position_history().get(key).value();
            //     auto position = std::format("{} {} {}", p.x, p.y, p.z);
            //
            //     ImGui::TableNextColumn();
            //     ImGui::Text("%u", key);
            //     // ImGui::TableNextColumn();
            //     // ImGui::Text(input.c_str());
            //     ImGui::TableNextColumn();
            //     ImGui::Text(position.c_str());
            // }

            ImGui::EndTable();
        }

        ImGui::End();

        bool change_imgui = false;

        m_input_manager.update();
        if(m_input_manager.is_quit()) {
            m_is_running = false;
        }
        m_world_controller.update(m_lockstep.delta_time());

        perf_stats.draw();
        packet_backlog.draw();
        network_stats.draw();
        tw::dbg::ComponentGui<tw::io::InputManager>().draw(&m_input_manager);

        m_world.step(m_lockstep.delta_time());

        m_world_renderer.render();
        FrameMark;
    }
}

}
