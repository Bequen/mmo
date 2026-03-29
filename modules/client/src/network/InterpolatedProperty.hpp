#pragma once

#include <array>
#include <chrono>

#include "common.hpp"

template<typename T, int length = 3>
class InterpolatedProperty {
    using Clock = std::chrono::high_resolution_clock;

    std::array<T, length> m_buffer;
    std::array<Clock::time_point, length> m_time_buffer;

public:
    InterpolatedProperty(T initial_value) {
        for(int i = 0; i < length; i++) {
            m_buffer[i] = initial_value;
            m_time_buffer[i] = Clock::now();
        }
    }

    GET_REF(m_buffer, values);
    GET_REF(m_time_buffer, times);

    void push(Clock::time_point point, T value) {
        if(point < m_time_buffer.at(0)) {
            return;
        }

        int i;
        for(i = 1; i < length; i++) {
            if(m_time_buffer.at(i) > point) {
                m_time_buffer[i - 1] = point;
                m_buffer[i - 1] = value;
                return;
            } else {
                m_time_buffer[i - 1] = m_time_buffer[i];
                m_buffer[i - 1] = m_buffer[i];
            }
        }

        m_time_buffer[i - 1] = point;
        m_buffer[i - 1] = value;
        return;

        // for(int i = 0; i < length; i++) {
        //     if(m_time_buffer.at(i) > point) {
        //         for(int o = 1; o < i; o++) {
        //             m_time_buffer[o - 1] = m_time_buffer[o];
        //             m_buffer[o - 1] = m_buffer[o];
        //         }
        //
        //         m_time_buffer[i] = point;
        //         m_buffer[i] = value;
        //
        //         return;
        //     }
        // }
        //
        // m_time_buffer[length] = point;
        // m_buffer[length] = value;

    }

    inline std::tuple<T, T, float> get_values_around(Clock::time_point point) const {
        T prev = m_buffer.at(0);
        Clock::time_point prev_point = m_time_buffer.at(0);

        for(int i = 0; i < length; i++) {
            if(m_time_buffer.at(i) > point) {
                float value;
                if(i == 0) {
                    value = 1.0f;
                } else {
                    value = (float)(point - prev_point).count() / (m_time_buffer[i] - prev_point).count();
                }

                return std::make_tuple(prev, m_buffer.at(i), value);
            }

            prev = m_buffer[i];
            prev_point = m_time_buffer[i];
        }

        return std::make_tuple(m_buffer[m_buffer.size() - 1], m_buffer[m_buffer.size() - 1], 1.0f);
    }
};
