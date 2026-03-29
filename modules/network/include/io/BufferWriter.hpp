#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Write.hpp"

namespace tw::net {

template<typename T>
class BufferWriter : public Write<T> {
private:
    Write<T>* m_writeable;
    std::vector<T> m_buffer;
    uint32_t m_head;

public:
    size_t remaining_size() {
        return m_buffer.size() - m_head;
    }

    BufferWriter(Write<T>* writeable, size_t buffer_size) :
        m_writeable(writeable),
        m_buffer(buffer_size),
        m_head(0)
    { }

    virtual size_t write(std::span<T> data) override {
        if(remaining_size() < data.size()) {
            size_t write_size = flush();
            write_size += m_writeable->write_into(data);
            m_head = 0;

            return write_size;
        }

        std::copy(data.begin(), data.end(), m_buffer.begin() + m_head);
        size_t write_size = data.size();

        m_head += data.size();

        return write_size;
    }

    virtual size_t flush() override {
        m_writeable->write_into(std::span<T>(m_buffer.begin(), m_buffer.begin() + m_head));
        size_t write_size = m_head;
        m_head = 0;

        return write_size;
    }
};

}
