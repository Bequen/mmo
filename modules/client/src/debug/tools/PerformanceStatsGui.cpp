#include "PerformanceStatsGui.hpp"

#include <imgui.h>
#include <implot.h>
#include <implot_internal.h>

namespace tw::dbg::tools {

PerformanceStatsGui::PerformanceStatsGui(LockStep& lock_step) :
    m_lockstep(lock_step),
    fps_history(1000),
    frame_idxs(1000)
{
}

void PerformanceStatsGui::draw() {
    ImGui::Begin("Stats");

    ImGui::Text("FPS: %ld", m_lockstep.fps());
    fps_history[fps_history_idx] = m_lockstep.fps();
    frame_idxs[fps_history_idx] = frame_idx++;

    if(fps_history_idx == fps_history.size() - 1) {
        is_plot_filled = true;
    }

    fps_history_idx = (fps_history_idx + 1) % fps_history.size();

    if(ImPlot::BeginPlot("FPS Plot")) {
        ImPlot::SetupAxes("FrameIdx","FPS", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
        ImPlot::SetupAxisLimits(ImAxis_X1, std::max((double)frame_idx - 1000.0, 0.0), frame_idx, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,0,120);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, 120);
        ImPlot::PlotLine("FPS", frame_idxs.data(), fps_history.data(), is_plot_filled ? (int)fps_history.size() : (int)fps_history_idx - 1);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

}
