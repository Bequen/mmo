#pragma once

#include "runtime/LockStep.hpp"

namespace tw::dbg::tools {

class PerformanceStatsGui {
private:
    LockStep& m_lockstep;

    std::vector<uint32_t> fps_history;
    std::vector<uint32_t> frame_idxs;

    uint32_t fps_history_idx = 0;
    uint32_t frame_idx = 0;
    bool is_plot_filled = false;

public:
    PerformanceStatsGui(LockStep& lock_step);

    void draw();
};

}
