#pragma once

#include "Read.hpp"

#include <spdlog/spdlog.h>
#include <vector>

namespace tw::net {

template<typename T>
class BufferReader : public Read<T> {
    Read<T>* m_readable;

    std::vector<T> m_buffer;

    size_t m_head;
    size_t m_tail;

    size_t remaining_size() {
        return m_head - m_tail;
    }

public:
    BufferReader(Read<T>* readable, size_t buffer_size) :
        m_readable(readable),
        m_buffer(buffer_size),
        m_head(0),
        m_tail(0) {

    }

    size_t read(std::span<T> target) override {
        size_t read_size = std::min(remaining_size(), target.size());
        std::copy(m_buffer.begin() + m_tail,
                m_buffer.begin() + m_tail + read_size,
                target.begin());

        spdlog::info("Read {} bytes", read_size);

        m_tail += read_size;

        // read next chunk
        if(m_tail == m_head && read_size < target.size()) {
            spdlog::info("Reading next chunk");
            m_head = m_readable->read(std::span<T>(m_buffer.begin(), m_buffer.end()));
            m_tail = 0;
        }

        if(target.size() > read_size && m_head > 0) {
            read_size += read(std::span<T>(target.begin() + read_size, target.end()));
        }

        return read_size;
    }

    std::optional<T> peek() {
        if(remaining_size() > 0) {
            return m_buffer[m_tail];
        }

        return std::nullopt;
    }
};

}
