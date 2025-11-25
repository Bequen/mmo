#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace tw::dbg::tools {

void draw_matrix_widget(const glm::mat4& mat, bool display_headers);

}
