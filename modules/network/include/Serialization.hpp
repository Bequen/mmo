#pragma once

#include "io/Write.hpp"
#include "io/Read.hpp"
#include <limits>
#include <span>
#include <spdlog/spdlog.h>

namespace tw::net {

class Serialization;

template<typename T>
class Serializer final {
public:
    static_assert(sizeof(T), "Invalid type for serialization");
    static bool serialize(tw::net::Serialization& buffer, T& value);
};


template<typename T>
concept Serializable = requires(tw::net::Serialization& b, T& t){
    Serializer<T>::serialize(b, t);
};

/**
 * Handles serialization of data
 */
class Serialization {
    union {
        Write<std::byte>* m_writeable;
        Read<std::byte>* m_readable;
    } m_io;

    bool m_is_reading;

    Serialization(Write<std::byte>* writeable) :
        m_io({
            .m_writeable = writeable
        }),
        m_is_reading(false)
    { }

    Serialization(Read<std::byte>* writeable) :
        m_io({
            .m_readable = writeable
        }),
        m_is_reading(true)
    { }

public:
    static Serialization reading(Read<std::byte>* read) {
        return Serialization(read);
    }

    static Serialization writing(Write<std::byte>* writeable) {
        return Serialization(writeable);
    }

    const bool is_reading() const {
        return m_is_reading;
    }

    template<Serializable T>
    bool serialize(T& value) {
        return Serializer<T>::serialize(*this, value);
    }



    /**
     * Special implementation that supports pointers. Useful for numeric types.
     */
    template<typename T>
    bool
    serialize(T* value, std::enable_if<std::numeric_limits<T>::is_integer, T>::type* = 0) {
        // TODO: enable only if trivially copyable
        // if constexpr (std::is_trivially_copyable_v<T>) {
        //     if(m_is_reading) {
        //         const std::string packet_name = typeid(T).name();
        //         return m_io.m_readable->read(std::as_writable_bytes(std::span{value, 1}));
        //     } else {
        //         auto bytes = std::as_writable_bytes(std::span{value, 1});
        //         size_t num_writen = m_io.m_writeable->write(bytes);

        //         spdlog::info("Pico");
        //         return true;
        //     }
        // } else {
        //     throw std::runtime_error("Cannot serialize non-trivially copyable type");
        // }

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

    bool serialize(char* buffer, size_t length) {
        // if(m_is_reading) {
        //     return m_io.m_readable->read(std::as_writable_bytes(std::span{buffer, length}));
        // } else {
        //     return m_io.m_writeable->write(std::as_writable_bytes(std::span{buffer, length}));
        // }
        return true;
    }

    void flush() {
        if(!m_is_reading) {
            m_io.m_writeable->flush();
        }
    }
};

}
