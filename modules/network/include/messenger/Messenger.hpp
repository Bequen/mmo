#pragma once

#include <immintrin.h>
#include <optional>
#include <span>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <tracy/Tracy.hpp>

#include "NetworkError.hpp"
#include "packets/Packet.hpp"
#include "MessageRegistry.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"
#include "tl/expected.hpp"

namespace tw::net {

template<typename TData, std::derived_from<Write<TData>> TOutput>
class Messenger {
private:
    const uint32_t MAX_MESG_BODY_SIZE = 65536;
    const uint32_t MESG_MAGIC = 0x1DEADBEE;

    TOutput m_stream;

    std::optional<PacketType> m_next_packet_type;

    bool m_is_skipping;
    uint32_t m_buffered_size;

    size_t m_mesg_size;
    size_t m_read_head;
    std::vector<std::byte> m_input_buffer;

public:
    Messenger(Messenger && m) :
      m_stream(std::move(m.m_stream)),
      m_next_packet_type(m.m_next_packet_type),
      m_input_buffer(std::move(m.m_input_buffer)),
      m_buffered_size(m.m_buffered_size),
      m_is_skipping(m.m_is_skipping),
      m_mesg_size(m.m_mesg_size),
      m_read_head(m.m_read_head)
    {
        // m_stream.set_non_blocking();
    }

    Messenger(TOutput&& stream) :
        m_stream(std::move(stream)),
        m_input_buffer(MAX_MESG_BODY_SIZE),
        m_buffered_size(0),
        m_is_skipping(false),
        m_mesg_size(0),
        m_read_head(0)
    {
        // m_stream.set_non_blocking();
    }

    Messenger<TData, TOutput> operator=(const Messenger<TData, TOutput>&) = delete;

    Messenger<TData, TOutput> operator=(Messenger<TData, TOutput>&& m) {
        m_stream = std::move(m.m_stream);
        m_next_packet_type = m.m_next_packet_type;
        m_input_buffer = std::move(m.m_input_buffer);
        m_buffered_size = m.m_buffered_size;
        m_is_skipping = m.m_is_skipping;
        m_mesg_size = m.m_mesg_size;
        m_read_head = m.m_read_head;
    }

    template <std::derived_from<google::protobuf::Message> T>
    tl::expected<size_t, NetworkError> send(T &content) {
        ZoneScopedN("Messenger::send");
        auto id = (int32_t)Message<T>::value;

        std::string payload;
        if(!content.SerializeToString(&payload)) {
            spdlog::error("Failed to serialize message");
            throw std::runtime_error("Serialization failed");
        }

        // spdlog::info("Sending {}: {}", (int)Message<T>::value, content.DebugString());

        int32_t length = payload.length();
        if(length == 0) {
            return 0;
        }

        // append encoded id & length before payload and write it to the stream
        //
        const uint32_t HEADER_SIZE = 4 + 4 + 4 + 4;

        std::string message;
        message.resize(HEADER_SIZE + payload.length());

        const uint32_t magic = 0xDEADBEEF;
        const uint32_t frame_type = quicr::FrameType::StreamBase;

        std::memcpy(message.data(), &magic, sizeof(magic));
        std::memcpy(message.data() + sizeof(magic), &frame_type, sizeof(frame_type));
        std::memcpy(message.data() + sizeof(frame_type) + sizeof(magic), &length, sizeof(length));
        std::memcpy(message.data() + sizeof(frame_type) + sizeof(magic) + sizeof(length), &id, sizeof(id));
        // std::memcpy(message.data() + sizeof(id) + sizeof(length), &MESG_MAGIC, sizeof(MESG_MAGIC));
        std::memcpy(message.data() + HEADER_SIZE, payload.data(), payload.length());

        auto write_result = m_stream.write(std::as_writable_bytes(std::span(message)));
        if(!write_result.has_value()) {
            return tl::make_unexpected(write_result.error());
        }

        return write_result.value();
    }

    int32_t m_packet_peek_size = 0;

    tl::expected<std::optional<PacketType>, NetworkError> peek() {
        ZoneScopedN("Messenger::peek");
        if(m_next_packet_type.has_value()) {
            return m_next_packet_type;
        }

        if(m_read_head < 4) {
            auto result = m_stream.read_into(std::as_writable_bytes(std::span{(char*)m_input_buffer.data(), sizeof(PacketType) - m_read_head}));

            if(!result.has_value()) {
                return tl::make_unexpected(result.error());
            }

            m_read_head += result.value();

            if(m_read_head < 4) {
                return {};
            }
        }

        if(m_read_head < 8) {
            auto result = m_stream.read_into(std::as_writable_bytes(std::span{(char*)m_input_buffer.data() + m_read_head, 8 - m_read_head}));

            if(!result.has_value()) {
                return tl::make_unexpected(result.error());
            }

            m_read_head += result.value();

            if(m_read_head < 8) {
                return {};
            }

            m_mesg_size = *reinterpret_cast<uint32_t*>(m_input_buffer.data() + 4);
        }

        if(m_read_head < m_mesg_size + 8) {
            if(m_mesg_size + 8 > m_input_buffer.size()) {
                return tl::make_unexpected(NetworkError(NetworkErrorType::NOT_ENOUGH_MEMORY));
            }

            auto result = m_stream.read_into(std::as_writable_bytes(std::span{(char*)m_input_buffer.data() + m_read_head, m_mesg_size + 8 - m_read_head}));

            if(!result.has_value()) {
                return tl::make_unexpected(result.error());
            }

            m_read_head += result.value();

            if(m_read_head < m_mesg_size + 8) {
                return {};
            }
        }

        m_next_packet_type = (PacketType)(*reinterpret_cast<int32_t*>(m_input_buffer.data()));
        return m_next_packet_type;
    }

    template<typename T>
    tl::expected<T, NetworkError> pop(size_t* out_size) {
        ZoneScopedN("Messenger::pop");
        T result = {};

        result.ParseFromArray(m_input_buffer.data() + 8, m_mesg_size);
        // spdlog::info("Received {}: {}", (int)m_next_packet_type.value(), result.DebugString());

        m_read_head = 0;
        m_mesg_size = 0;
        m_next_packet_type = {};


        return result;
    }

    void skip() {

    }

    void clear() {
        m_next_packet_type = std::nullopt;

        int message_length = 0;
        int size = sizeof(message_length);

        // m_stream.read_exact(std::as_writable_bytes(std::span{&message_length, 1}));

        std::vector<char> data(message_length);
        // m_stream.read_exact(std::as_writable_bytes(std::span{data.data(), (size_t)message_length}));

        // m_input_buffer.reset();
    }
};

}
