#pragma once

#include <imgui.h>

#include "debug/ComponentGui.hpp"
#include "world/CharacterController.hpp"

template<>
class tw::dbg::ComponentGui<tw::CharacterController> {
public:
    void draw(tw::CharacterController* instance) {
        ImGui::SeparatorText("Character Controller Component");

        if(ImGui::BeginTable("history", 3)) {
            for(auto key : instance->input_history().buffer()) {
                // ImGui::TableNextRow();

                // ImGui::TableNextColumn();
                // ImGui::Text("%u", key.first);

                // ImGui::TableNextColumn();
                // if(instance->input_history().get(key.first).has_value()) {
                //     glm::vec3 input_vec = *instance->input_history().get(key.first).value();
                //     std::string input = std::format("{} {} {}", input_vec.x, input_vec.y, input_vec.z);
                //     ImGui::Text("%s", input.c_str());
                // } else {
                //     ImGui::Text("None");
                // }

                // ImGui::TableNextColumn();
                // if(instance->position_history().get(key).has_value()) {
                //     glm::vec3 position_vec = *instance->position_history().get(key).value();
                //     std::string position = std::format("{} {} {}", position_vec.x, position_vec.y, position_vec.z);
                //     ImGui::Text("%s", position.c_str());
                // } else {
                //     ImGui::Text("None");
                // }
            }

            ImGui::EndTable();
        }
    }
};
