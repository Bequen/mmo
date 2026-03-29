#pragma once

#include <imgui.h>
#include <implot.h>
#include <implot_internal.h>

#include "metrics/BucketMetric.hpp"

namespace tw::dbg::tools {

template<typename T, typename Interval, typename Op = net::SumOp<T>>
class MetricWidget {
    std::string m_name;

    net::BucketMetric<T, Interval, Op>& m_metric;

    using Self = MetricWidget<T, Interval, Op>;

    struct {
        T constraint_from;
        T constraint_to;

        T from;
        T to;
    } y_axis;
    
    bool m_is_scrolling = true;

public:
    MetricWidget(
            const std::string& name,
            net::BucketMetric<T, Interval, Op>& metric
    ) :
        m_name(name),
        m_metric(metric)
    {
        y_axis = {
            .constraint_from = 0,
            .constraint_to = 500,
            .from = 0,
            .to = 250
        };
    }

    Self& set_x_axis_limits_contraints(T from, T to) {
        ImPlot::SetupAxisLimits(ImAxis_X1, from, to);
        return *this;
    }

    Self& set_y_axis_limits(double from, double to) {
        return *this;
    }

    Self& enable_scrolling() {
        m_is_scrolling = true;
    }

    Self& disable_scrolling() {
        m_is_scrolling = true;
    }

    
    void draw() {
        ImGui::PushID(m_name.c_str());

        auto head = m_metric.get_head();
        auto head_timeline = m_metric.get_head_timeline();

        auto tail = m_metric.get_tail();
        auto tail_timeline = m_metric.get_tail_timeline();

        static float m_metric_history = 10.0f;
        ImGui::Checkbox("Is Scrolling", &m_is_scrolling);
        if(m_is_scrolling) {
            ImGui::SliderFloat("History", &m_metric_history,1,30,"%.1f s");
        }

        ImGui::Text("Min: %i", m_metric.min());
        ImGui::Text("Max: %i", m_metric.max());

        if(ImPlot::BeginPlot(m_name.c_str())) {
            auto from = tail_timeline.empty() ? *(head_timeline.end() - 1) : *(tail_timeline.end() - 1);

            ImPlot::SetupAxes("Time", m_name.c_str(), ImPlotAxisFlags_None, ImPlotAxisFlags_None);
            if(m_is_scrolling) {
                ImPlot::SetupAxisLimits(ImAxis_X1, from - m_metric_history, from, ImGuiCond_Always);
            }

            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, m_metric.max() * 2, ImGuiCond_Always);
            // ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, 10000);

            ImPlot::PlotLine(m_name.c_str(), head_timeline.data(), head.data(), head.size());
            ImPlot::PlotLine(m_name.c_str(), tail_timeline.data(), tail.data(), tail.size());
            ImPlot::EndPlot();
        }

        ImGui::PopID();
    }
};

}
