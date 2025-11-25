#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <print>

namespace tw::net {

template<typename T>
struct AverageOp {
    uint32_t count;
    T sum;

    AverageOp() :
    count(0),
    sum({}) {
    }

    void add(const T value) {
        sum += value;
        count++;
    }

    T result() const {
        return count == 0 ? 0 : sum / count;
    }
};

template<typename T>
struct SumOp {
    T sum;

    void add(const T value) {
        sum += value;
    }

    T result() const {
        return sum;
    }
};

template<typename T, typename Interval,
    typename Operation = SumOp<T>,
    typename Clock = std::chrono::steady_clock>
class BucketMetric {

    T m_min, m_max;

    std::vector<T> m_metric;
    std::vector<uint32_t> m_bucket_idx;

    Operation m_op;

    uint32_t m_offset;
    uint32_t m_right, m_left;

    std::string m_format;

    const uint32_t get_bucket(Clock::time_point time_point) const {
        return std::chrono::floor<Interval>(time_point).time_since_epoch().count() - m_offset;
    }

public:
    BucketMetric(std::string format, uint32_t size) : 
        m_metric(size),
        m_bucket_idx(size),
        m_right(0), m_left(0),
        m_offset(0),
        m_format(format)
    {
        m_offset = get_bucket(Clock::now());
    }

    const T max() const {
        return m_max;
    }

    const T min() const {
        return m_min;
    }

    const std::string& format() const {
        return m_format;
    }

    size_t max_size() const {
        return m_metric.size();
    }

    void push(T value) {
        auto time = Clock::now();
        size_t bucket = get_bucket(time) % m_metric.size();
        size_t idx = get_bucket(time);

        // set result to correct bucket
        if(m_right != bucket) {
            m_metric[m_right] = m_op.result();
            m_min = std::min(m_min, m_op.result());
            m_max = std::max(m_max, m_op.result());

            m_bucket_idx[m_right] = idx++;
            m_right++;
            m_op = {};
        }

        // reset all buckets until the required one
        for(; m_right != bucket; m_right = (m_right + 1) % m_metric.size()) {
            m_metric[m_right] = {};
            m_bucket_idx[m_right] = idx++;
            if(m_right == m_left) {
                m_left = (m_left + 1) % m_metric.size();
            }
        }

        m_op.add(value);
    }

    const size_t get_size() const {
        return m_right - m_left + (m_left > m_right ? m_metric.size() : 0);
    }

    const T get(uint32_t idx) const {
        if(idx > get_size()) {
            throw std::invalid_argument("`idx` cannot be higher than buffer size");
        }

        return m_metric[m_left + idx].result();
    }

    std::span<T> get_head() {
        return std::span(m_metric).subspan(m_left, (m_right > m_left ? m_right : m_metric.size()));
    }

    std::span<uint32_t> get_head_timeline() {
        return std::span(m_bucket_idx).subspan(m_left, (m_right > m_left ? m_right : m_bucket_idx.size()));
    }

    std::span<T> get_tail() {
        if(m_right > m_left) {
            return std::span<T>();
        }

        return std::span(m_metric).subspan(0, m_right);
    }

    std::span<uint32_t> get_tail_timeline() {
        if(m_right > m_left) {
            return std::span<T>();
        }

        return std::span(m_bucket_idx).subspan(0, m_right);
    }
};

}
