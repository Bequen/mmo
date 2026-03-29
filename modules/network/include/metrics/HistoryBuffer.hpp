
#pragma once

#include "common.hpp"

#include <optional>
#include <string>
#include <vector>

/**
 * Stores history of a property for data analysis
 */
template<typename TKey, typename TValue>
class HistoryBuffer {
private:
    size_t m_head, m_tail;
    std::vector<std::pair<TKey, TValue>> m_buffer;

public:
    size_t size() const {
        if(m_tail > m_head) {
            return max_size() - m_tail + m_head;
        }

        return (m_head - m_tail) % max_size();
    }

    size_t max_size() const { return m_buffer.size(); }

    bool is_full(float percentage) {
        return size() / (float)max_size() > percentage;
    }

    void clear() {
        m_tail = m_head = 0;
    }

    GET_REF(m_buffer, buffer);

    HistoryBuffer(TKey default_key, TValue&& default_value, size_t size) :
        m_head(0), m_tail(0), m_buffer(size) {
    }

    std::optional<const TValue*> get(TKey key) const {
        for(size_t i = m_tail; i != m_head; (i++) % max_size()) {
            if(m_buffer[i].first > key) {
                return {};
            }

            if(m_buffer[i].first == key) {
                return &m_buffer[i].second;
            }
        }

        return {};
    }

    bool set(TKey key, const TValue& value) {
        if(key < m_buffer.at(m_tail).first) {
            return false;
        }

        int i = m_tail + 1;
        if(m_tail != m_head) {
            for(i = m_tail + 1; i != m_head; i++) {
                if(m_buffer.at(i).first > key) {
                    m_buffer[(i - 1) % max_size()] = std::make_pair(key, value);
                    m_head++;
                    return true;
                } else {
                    m_buffer[(i - 1) % max_size()] = m_buffer[i];
                }
            }
        }

        m_buffer[(i - 1) % max_size()] = std::make_pair(key, value);
        m_head = (m_head + 1) % max_size();
        return true;
    }
};
