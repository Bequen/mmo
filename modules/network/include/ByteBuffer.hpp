#pragma once

#include <limits>
#include <print>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <cstring>


namespace tw::net {

class ByteBuffer;

template<typename T>
class Serializer final { 
public:
    static_assert(sizeof(T), "Invalid type for serialization");
    static bool serialize(ByteBuffer& buffer, T& value);
};


template<typename T>
concept Serializable = requires(ByteBuffer& b, T& t){
    Serializer<T>::serialize(b, t);
};

#define SERIALIZER(class_name, buffer, message) \
    template<> bool tw::net::Serializer<class_name>::serialize(ByteBuffer& buffer, class_name& message)

class ByteBuffer {
    bool m_is_reading;

    std::vector<uint8_t> m_bytes;
    uint32_t m_writing_head;
    uint32_t m_reading_head;

public:
    uint32_t writing_head() const {
        return m_writing_head;
    }

    uint32_t reading_head() const {
        return m_reading_head;
    }

    void set_for_read() {
        m_is_reading = true;
    }

    void set_for_write() {
        m_is_reading = false;
    }

    uint32_t max_size() const {
        return m_bytes.size();
    }

    uint32_t size() const {
        return m_writing_head;
    }

    int32_t remaining() const {
        if(is_reading()) {
            return size() - m_reading_head;
        } else {
            return max_size() - m_writing_head;
        }
    }

    void reset_write_head() {
        m_writing_head = 0;
    }

    void move_write_head(int32_t relative_position) {
        m_writing_head += relative_position;
    }

    void reset() {
        m_writing_head = 0;
        m_reading_head = 0;
    }

    std::vector<uint8_t>& data() {
        return m_bytes;
    }

    const std::vector<uint8_t>& data() const {
        return m_bytes;
    }

    bool is_reading() const {
        return m_is_reading;
    }

    ByteBuffer(size_t size) :
        m_is_reading(false),
        m_bytes(size),
        m_writing_head{0},
        m_reading_head{0} {
    }


    template<Serializable T>
    inline bool write(T& value) {
        m_is_reading = false;
        uint32_t last_position = m_writing_head;

        if(!Serializer<T>::serialize(*this, value)) {
            m_writing_head = last_position;
            return false;
        }

        return true;
    }

    template<Serializable T>
    inline bool read(T& value) {
        m_is_reading = true;
        uint32_t last_position = m_reading_head;

        if(!Serializer<T>::serialize(*this, value)) {
            m_reading_head = last_position;
            return false;
        }

        return true;
    }

    bool bytes(char* values, size_t num_bytes) {
        if(remaining() < num_bytes) {
            return false;
        }

        if(m_is_reading) {
            std::memcpy(values, &m_bytes.data()[m_reading_head], num_bytes);
            m_reading_head += num_bytes;
        } else {
            std::memcpy(&m_bytes.data()[m_writing_head], values, num_bytes);
            m_writing_head += num_bytes;
        }

        return true;
    }

    template<Serializable T>
    inline bool serialize(T& value) {
        uint32_t last_position = m_writing_head;
        if(!Serializer<T>::serialize(*this, value)) {
            m_writing_head = last_position;
            return false;
        }

        return true;
    }

    template<typename T>
    std::enable_if_t<std::numeric_limits<T>::is_integer, bool>
    serialize(T* target) {
        if(remaining() < sizeof(T)) {
            return false;
        }

        if(m_is_reading) {
            *target = *(T*)(m_bytes.data() + m_reading_head);
            m_reading_head += sizeof(*target);
        } else {
            *(T*)(m_bytes.data() + m_writing_head) = *target;
            m_writing_head += sizeof(*target);
        }

        return true;
    }

    bool serialize(float* target) {
        union {
            float floating;
            unsigned int integer;
        } float_val;
        float_val.floating = *target;
        bool result = serialize(&float_val.integer);
        *target = float_val.floating;

        return result;
    }

    void print() {
        for(int i = 0; i < size(); i++) {
            std::print("|{:#04x}|", data()[i]);
            if(i > 0 && i % 10 == 0) {
                std::println();
            }
        } std::println("----------");
    }
};

}
