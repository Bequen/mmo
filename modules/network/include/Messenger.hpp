#pragma once

#include "ByteBuffer.hpp"
#include "NetworkResult.hpp"
#include "StatsLogger.hpp"
#include "TcpSocket.hpp"
#include "exception/SocketClosedException.hpp"
#include "packets/Packet.hpp"
#include <optional>

namespace tw::net {

class Messenger {
private:
    TcpSocket m_socket;
    ByteBuffer m_input_buffer;
    ByteBuffer m_output_buffer;

    std::optional<PacketType> m_next_packet_type;

    bool m_is_closed;

public:
    const bool is_closed() const {
        return m_is_closed;
    }

    Messenger(TcpSocket&& socket) :
        m_socket(std::move(socket)),
        m_input_buffer(1024),
        m_output_buffer(1024),
        m_is_closed(false)
    {
    }

    size_t remaining() {
        return m_input_buffer.remaining();
    }

    void fetch() {
        if(m_is_closed) return;

        m_input_buffer.set_for_write();
        m_input_buffer.reset();

        auto result = m_socket.read_bytes_into(m_input_buffer);
        if(!result.is_ok()) {
            m_is_closed = true;
        }


        m_input_buffer.set_for_read();
    }

    template<typename T>
    void send(T& content) {
        if(m_is_closed) return;

        m_output_buffer.set_for_write();
        m_output_buffer.reset();

        m_output_buffer.serialize(Message<T>::value);
        m_output_buffer.serialize(content);

        auto result = m_socket.send_bytes_from(m_output_buffer);
        if(!result.is_ok()) {
            if(result.is(EPIPE)) {
                m_is_closed = true;
            }
        }

        if(m_socket.target().has_value()) {
            NetworkStatsLogger::instance()->log_send(Message<T>::value,
                    m_socket.target().value(), m_output_buffer);
        }
    }

    

    std::optional<PacketType> peek() {
        if(!m_next_packet_type.has_value() && m_input_buffer.remaining()) {
            int type;
            m_input_buffer.set_for_read();
            m_input_buffer.serialize(&type);
            m_next_packet_type = (PacketType)type;
        }

        return m_next_packet_type;
    }

    template<typename T>
    T pop() {
        T result = {};
        m_input_buffer.read(result);

        m_next_packet_type = {};

        if(m_socket.target().has_value()) {
            NetworkStatsLogger::instance()->log_receive(Message<T>::value,
                    m_socket.target().value(), m_input_buffer);
        }

        return result;
    }

    void clear() {
        m_next_packet_type = {};
        m_input_buffer.reset();
    }
};

}
