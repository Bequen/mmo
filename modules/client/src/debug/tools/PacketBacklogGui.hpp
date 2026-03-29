#pragma once

#include <format>
#include <vector>

#include "imgui.h"

#include "metrics/NetworkStatsLogger.hpp"

namespace tw::dbg::tools {

class PacketBacklogGui {
private:
    std::vector<uint32_t> m_buckets;
    uint32_t m_last_backlog_idx;

public:
    void draw() {
        // auto* instance = net::NetworkStatsLogger::instance();
        // size_t size = instance->get_size();

        // if(ImGui::BeginTable("Network Packets", 5)) {
        //     ImGui::TableSetupColumn("Message Type");
        //     ImGui::TableSetupColumn("Time");
        //     ImGui::TableSetupColumn("Is From Us");
        //     ImGui::TableSetupColumn("Target");
        //     ImGui::TableSetupColumn("Size");

        //     for(int32_t i = size-1; i >= 0; i--) {
        //         auto& item = instance->get_item(i);
        //         ImGui::PushID(item.timepoint.time_since_epoch().count());

        //         ImGui::TableNextRow();

        //         ImGui::TableNextColumn();
        //         ImGui::Text("%i", item.message_type);

        //         ImGui::TableNextColumn();
        //         ImGui::Text(std::format("{}", item.timepoint.time_since_epoch()).c_str());

        //         ImGui::TableNextColumn();
        //         ImGui::Checkbox("is_sent_from_us", &item.is_sent_by_us);

        //         ImGui::TableNextColumn();
        //         ImGui::Text(item.target.to_string().c_str());

        //         ImGui::TableNextColumn();
        //         ImGui::Text("%ld", item.buffer.size());

        //         ImGui::PopID();
        //     }
        //     ImGui::EndTable();
        // }
    }
};

}
