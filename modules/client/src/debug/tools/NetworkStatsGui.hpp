#pragma once

#include "BucketMetric.hpp"
#include "StatsLogger.hpp"
#include "debug/tools/MetricWidget.hpp"

#include <implot.h>
#include <implot_internal.h>

namespace tw::dbg::tools {

class NetworkStatsGui {
private:
    MetricWidget<uint32_t, std::chrono::seconds, net::AverageOp<uint32_t>> m_ping_widget;
    MetricWidget<uint32_t, std::chrono::seconds> m_outgoing_widget;
    MetricWidget<uint32_t, std::chrono::seconds> m_incoming_widget;

public:
    NetworkStatsGui() :
        m_ping_widget("Ping", net::NetworkStatsLogger::instance()->ping()),
        m_outgoing_widget("Outgoing", net::NetworkStatsLogger::instance()->outgoing()),
        m_incoming_widget("Incoming", net::NetworkStatsLogger::instance()->incoming())
    {
    }

    void draw() {
        auto* instance = net::NetworkStatsLogger::instance();
        auto& ping = instance->ping();

        ImGui::Begin("Network Stats");

        /* auto head = ping.get_head();
        auto head_timeline = ping.get_head_timeline();

        auto tail = ping.get_tail();
        auto tail_timeline = ping.get_tail_timeline();

        static float ping_history = 10.0f;
        ImGui::SliderFloat("Ping History", &ping_history,1,30,"%.1f s");

        if(ImPlot::BeginPlot("Ping")) {
            auto from = tail_timeline.empty() ? *(head_timeline.end() - 1) : *(tail_timeline.end() - 1);

            ImPlot::SetupAxes("FrameIdx","FPS", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_X1, from - ping_history, from, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 120);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, 10000);

            ImPlot::PlotLine("Ping", head_timeline.data(), head.data(), head.size());
            ImPlot::PlotLine("Ping", tail_timeline.data(), tail.data(), tail.size());
            ImPlot::EndPlot();
        } */

        m_ping_widget.draw();
        m_outgoing_widget.draw();
        m_incoming_widget.draw();

        ImGui::End();
    }
};

}
