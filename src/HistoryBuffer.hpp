
#pragma once

#include "common.hpp"

#include <array>
#include <optional>

template<typename TKey, typename TValue, int Size>
class HistoryBuffer {
private:
    std::array<TKey, Size> m_keys;
    std::array<TValue, Size> m_values;

public:
    GET_REF(m_keys, keys);
    GET_REF(m_values, values);

    HistoryBuffer(TKey default_key, TValue&& default_value) {
        for(int i = 0; i < Size; i++) {
            m_keys[i] = default_key;
            // m_values[i] = std::move(default_value);
        }
    }

    std::optional<const TValue*> get(TKey key) const {

        for(int i = 0; i < Size; i++) {
            if(m_keys[i] > key) {
                return {};
            }

            if(m_keys[i] == key) {
                return &m_values[i];
            }
        }

        return {};
    }

    void set(TKey key, const TValue& value) {
        if(key < m_keys.at(0)) {
            return;
        }

        int i;
        for(i = 1; i < Size; i++) {
            if(m_keys.at(i) > key) {
                m_keys[i - 1] = key;
                m_values[i - 1] = value;
                return;
            } else {
                m_keys[i - 1] = m_keys[i];
                m_values[i - 1] = m_values[i];
            }
        }

        m_keys[i - 1] = key;
        m_values[i - 1] = value;
        return;
    }


    void set(TKey key, TValue&& value) {
        if(key < m_keys.at(0)) {
            return;
        }

        int i;
        for(i = 1; i < Size; i++) {
            if(m_keys.at(i) > key) {
                m_keys[i - 1] = std::move(key);
                m_values[i - 1] = std::move(value);
                return;
            } else {
                m_keys[i - 1] = std::move(m_keys[i]);
                m_values[i - 1] = std::move(m_values[i]);
            }
        }

        m_keys[i - 1] = std::move(key);
        m_values[i - 1] = std::move(value);
    }
};
