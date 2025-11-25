#pragma once

#include <imgui.h>
#include <chrono>
#include <format>
#include <ratio>

#include "debug/ComponentGui.hpp"
#include "world/EntityInterpolation.hpp"

template<>
class tw::dbg::ComponentGui<EntityInterpolation> {
public:
    void draw(EntityInterpolation* instance) {
        ImGui::SeparatorText("Entity Interpolation");

        if(ImGui::BeginTable("entityInterpolation", 2)) {
            for(int i = 0; i < instance->values().size(); i++) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                auto value = instance->values()[i];
                ImGui::Text("%f %f %f", value.x, value.y, value.z);

                auto point = instance->times()[i].time_since_epoch();
                auto hours = std::chrono::duration_cast<std::chrono::hours>(point);
                point -= hours;
                auto minutes = std::chrono::duration_cast<std::chrono::minutes>(point);
                point -= minutes;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(point);
                point -= seconds;
                auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(point);

                ImGui::TableNextColumn();
                auto fmt = std::format("{}:{}:{}.{}", hours, minutes, seconds, milliseconds);
                ImGui::Text(fmt.c_str());
            }
            ImGui::EndTable();
        }
    }
};
