#pragma once

#include <chrono>
#include <thread>
#include <print>

namespace tw {

/**
 * Controls thread sleep to run exactly `n` times per second
 */
class LockStep {
    using Clock = std::chrono::steady_clock;
    using Millis = std::chrono::microseconds;
    const uint64_t IN_SECOND = std::chrono::duration_cast<Millis>(std::chrono::seconds(1)).count();

    uint64_t milliseconds;

    std::chrono::time_point<Clock> m_last_point;

    uint64_t m_last_interval_ms;

    bool m_popped;

public:
    LockStep(uint32_t steps_per_second) {
        m_last_point = Clock::now();
        set_steps_per_second(steps_per_second);
    }

    inline uint64_t milliseconds_per_frame() const {
        return milliseconds;
    }

    inline void set_steps_per_second(uint32_t steps_per_second) {
        milliseconds = IN_SECOND / steps_per_second;
    }

    inline uint64_t wait_time_in_ms() {
        return m_last_interval_ms;
    }

    inline uint64_t fps() {
        if(m_last_interval_ms == 0) {
            return IN_SECOND;
        }
        return IN_SECOND / m_last_interval_ms;
    }

    inline double delta_time() const {
        return m_last_interval_ms / (double)IN_SECOND;
    }

    bool update() {
        auto now = Clock::now();

        m_last_interval_ms = duration_cast<Millis>(now - m_last_point).count();

        if(m_last_interval_ms >= milliseconds) {
            m_last_point = now;
            return true;
        }

        return false;
    }
    
    /**
     * Waits if time interval from previous call is less than time between two steps
     */
    bool wait_for_next_step() {
        auto now = Clock::now();

        m_last_interval_ms = duration_cast<Millis>(now - m_last_point).count();

        if(m_last_interval_ms < milliseconds) {
            std::this_thread::sleep_for(Millis(milliseconds - m_last_interval_ms));
            return true;
        }

        m_last_point = now;
        return false;
    }
};

}
