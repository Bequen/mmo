#include "EntityManagerGui.hpp"

#include "debug/tools/MatrixWidget.hpp"
#include "debug/EntityInterpolationDebug.hpp"
#include "debug/CharacterBodyDebug.hpp"
#include "debug/CharacterControllerDebug.hpp"

#include "network/EntityInterpolation.hpp"
#include "world/CharacterBody.hpp"
#include "world/CharacterController.hpp"
#include "world/Transform.hpp"
#include "world/WorldEntity.hpp"
#include "draw/Mesh.hpp"

#include <imgui.h>
#include <entt/entity/entity.hpp>
#include <spdlog/spdlog.h>

namespace tw::dbg::tools {

EntityManagerGui::EntityManagerGui(World* world) :
    m_world(world) {

}

void draw_transform_controls(Transform* transform) {

}

void EntityManagerGui::draw_entity_components() {
    ImGui::Text("Entity ID: %d", (uint32_t)m_selected);

    ImGui::BeginChild("Transform Component", ImVec2(0, 0), ImGuiChildFlags_Border);

    auto transform = m_world->registry().try_get<Transform>(m_selected);

    if(transform != nullptr) {
        ImGui::SeparatorText("Transform Component");
        draw_matrix_widget(transform->transform, true);

        draw_transform_controls(transform);
    }


    auto mesh = m_world->registry().try_get<tw::drw::Mesh>(m_selected);
    if(mesh != nullptr) {
        ImGui::SeparatorText("Mesh Component");
        ImGui::Text("Num vertices: %i", mesh->num_vertices());
        ImGui::Text("Vertex offset: %i", mesh->vertex_offset());
    }

    draw_debug_components<tw::CharacterBody, tw::CharacterController, tw::net::EntityPositionInterpolation>();

    // auto character = m_world->registry().try_get<tw::CharacterBody>(m_selected);
    // if(character != nullptr) {
    //     tw::dbg::ComponentGui<tw::CharacterBody>().draw(character);
    // }
    //
    // auto entity_interpolation = m_world->registry().try_get<EntityInterpolation>(m_selected);
    // if(entity_interpolation != nullptr) {
    //     tw::dbg::ComponentGui<EntityInterpolation>().draw(entity_interpolation);
    // }

    ImGui::EndChild();
}

void EntityManagerGui::draw() {
    ImGui::Begin("Transforms");

    ImGui::BeginChild("Entities", ImVec2(0, 260), ImGuiChildFlags_Border);

    ImGui::SeparatorText("Entities");

    auto view = m_world->registry().view<const WorldEntity>();

    view.each([&](const auto entity, const WorldEntity& info) {
            ImGui::PushID((uint32_t)info.entity_id);
            if(ImGui::Selectable(info.name.c_str(), m_selected_entity == info.entity_id)) {
                m_selected_entity = info.entity_id;
                m_selected = entity;
            }
            ImGui::PopID();
    });

    ImGui::EndChild();

    if(m_selected_entity.has_value()) {
        draw_entity_components();
    }

    ImGui::End();
}

}
