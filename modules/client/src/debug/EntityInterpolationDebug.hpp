#pragma once

#include <imgui.h>
#include <chrono>
#include <format>
#include <ratio>

#include "debug/ComponentGui.hpp"
#include <implot.h>
#include <implot_internal.h>
#include "network/EntityInterpolation.hpp"

template<>
class tw::dbg::ComponentGui<tw::net::EntityPositionInterpolation> {
public:
    void draw(net::EntityPositionInterpolation* instance) {
        ImGui::SeparatorText("Entity Interpolation");

        bool m_is_scrolling = true;
        static float m_metric_history = 10.0f;
        ImGui::Checkbox("Is Scrolling", &m_is_scrolling);
        if(m_is_scrolling) {
            ImGui::SliderFloat("History", &m_metric_history,1,30,"%.1f s");
        }

        static int32_t time_buffer_len = 1000;
        ImGui::SliderInt("Buffer length (ms)", &time_buffer_len, 0, 1000);

        auto now = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(time_buffer_len);
        auto [from, to, mix] = instance->get_values_around(now);
        auto mixed_value = glm::mix(from, to, mix);

        ImGui::Text("Mixed value: (%f, %f)", mixed_value.x, mixed_value.y);

        if(ImPlot::BeginPlot("Interpolation")) {
            auto plot_x_from = instance->times().begin()->time_since_epoch().count();
            std::vector<float> times(instance->times().max_size());
            for(auto it = instance->times().begin(); it != instance->times().end(); ++it) {
                times.push_back(it->time_since_epoch().count() - plot_x_from);
            }

            float interpolated_time = now.time_since_epoch().count() - plot_x_from;
            std::vector<float> values(instance->values().max_size());
            for(auto it = instance->values().begin(); it != instance->values().end(); ++it) {
                values.push_back(it->x);
            }
            values.push_back(mixed_value.x);

            ImPlot::PlotScatter("Server", times.data(), values.data(), times.size() - 1);
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6, ImPlot::GetColormapColor(1), IMPLOT_AUTO, ImPlot::GetColormapColor(1));
            ImPlot::PlotScatter("Client", &interpolated_time, &mixed_value.x, 1);
            ImPlot::PopStyleVar();
            ImPlot::EndPlot();
        }

        if(ImGui::BeginTable("entityInterpolation", 2)) {
            for(int i = 0; i < instance->values().size(); i++) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                auto value = instance->values()[i];
                auto now = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(time_buffer_len);
                if(instance->times()[i] > now && (i == instance->values().size() - 1 || instance->times()[i + 1] <= now)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.65f)));
                    ImGui::Text("%f %f %f", mixed_value.x, mixed_value.y, mixed_value.z);
                    ImGui::TableNextColumn();
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)));
                }

                ImGui::Text("%f %f %f", value.x, value.y, value.z);

                auto point = instance->times()[i].time_since_epoch();
                auto hours = std::chrono::duration_cast<std::chrono::hours>(point);
                point -= hours;
                hours %= 24;
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
