#pragma once

#include <imgui.h>

#include "debug/ComponentGui.hpp"
#include "world/CharacterBody.hpp"

template<>
class tw::dbg::ComponentGui<tw::CharacterBody> {
public:
    void draw(CharacterBody* instance) {
        ImGui::SeparatorText("Character Body Component");

        auto position = instance->m_character->GetPosition();
        ImGui::InputFloat3("Position", (float*)&position);

        auto new_velocity = instance->m_character->GetLinearVelocity();
        ImGui::InputFloat3("New Velocity", (float*)&new_velocity);
    }
};
