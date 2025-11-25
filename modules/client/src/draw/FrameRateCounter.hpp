#pragma once

#include <chrono>
#include <ratio>

class FrameRateCounter {
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    TimePoint m_last_frame;

    double m_frame_count;

public:
    uint64_t frame_count() {
        return (uint64_t)m_frame_count;
    }

    void next_frame() {
        TimePoint current = std::chrono::high_resolution_clock::now();

        m_frame_count = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(current - m_last_frame).count();

        m_last_frame = current;
    }
};
