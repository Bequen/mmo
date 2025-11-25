#include "MatrixWidget.hpp"

namespace tw::dbg::tools {

void draw_matrix_widget(const glm::mat4 &mat, bool display_headers) {
    if(ImGui::BeginTable("transform_component", 4, ImGuiTableFlags_Borders)) {
        if(display_headers) {
            ImGui::TableSetupColumn("Right");
            ImGui::TableSetupColumn("Front");
            ImGui::TableSetupColumn("Up");
            ImGui::TableSetupColumn("Translation");
            ImGui::TableHeadersRow();
        }

        for(int row = 0; row < 4; row++) {
            ImGui::TableNextRow();

            for(int col = 0; col < 4; col++) {
                ImGui::TableSetColumnIndex(col);
                ImGui::Text("%f", mat[col][row]);
            }
        }
        ImGui::EndTable();
    }
}

}
