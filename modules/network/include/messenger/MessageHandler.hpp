#pragma once

#include <concepts>
#include <functional>
#include <google/protobuf/message.h>
#include <span>

#include "Address.hpp"
#include "Messenger.hpp"
#include "NetworkError.hpp"
#include "TcpStream.hpp"
#include "packets/Packet.hpp"
#include "packets/LoginPacket.hpp"
#include "protocol/quicr/QuicrConnection.hpp"

namespace tw::net {

/**
 * Contains handlers for each message type. Calls this handler when message comes in.
 */
class MessageHandler {
private:
    // std::optional<Messenger<std::byte, quicr::QuicrConnection>> m_quicr_messenger;
    // Messenger<std::byte, TcpStream> m_server_messenger;
    std::unique_ptr<quicr::QuicrEndpoint> m_quicr_endpoint;
    quicr::QuicrConnection* m_quicr_connection;

    std::vector<std::function<tl::expected<void, NetworkError>(std::span<std::byte>)>> m_handlers;

    std::unique_ptr<quicr::QuicrEndpoint> create_endpoint() {
        auto endpoint_r = quicr::QuicrEndpoint::create();
        if(!endpoint_r) {
            spdlog::error("Failed to create QuicrEndpoint: {}", endpoint_r.error().message());
            throw std::runtime_error("Failed to create QuicrEndpoint");
        }

        return std::make_unique<quicr::QuicrEndpoint>(std::move(endpoint_r.value()));
    }

public:
    const bool is_connected() const {
        return m_quicr_connection->state() == quicr::QuicrConnectionState::Established;
    }

    MessageHandler(MessageHandler&& m)
        // : m_server_messenger{std::move(m.m_server_messenger)},
      :
      m_handlers(std::move(m.m_handlers)),
      m_quicr_endpoint(std::move(m.m_quicr_endpoint)),
      m_quicr_connection(m.m_quicr_connection) {

    }

    MessageHandler(Address address) :
        m_quicr_endpoint(create_endpoint()),
        m_quicr_connection(m_quicr_endpoint->connect(address).value()),
        m_handlers(100) {
        spdlog::info("Connected to server at {}", address.to_string());
    }


    // MessageHandler(Messenger<std::byte, TcpStream>&& server_messenger) :
    //     // m_server_messenger{std::move(server_messenger)},
    //     m_quicr_connection(std::move(server_messenger.connection())),
    //     m_handlers(100) {

    // }

    template<typename T>
    constexpr void set_handler(const std::function<void(T*)> handler) {
        PacketType type = Message<T>::value;
        m_handlers[type] = [handler, this](std::span<std::byte> data) -> tl::expected<void, NetworkError> {
            T result = {};

            result.ParseFromArray(data.data(), data.size());
            // spdlog::info("Deserialized message [{}]: {}", (int32_t)Message<T>::value, result.DebugString());

            handler(&result);
            // if(m_server_messenger.peek().has_value() && m_server_messenger.peek().value() == Message<T>::value) {
            //     tl::expected<T, NetworkError> mesg = m_server_messenger.pop<T>(nullptr);
            //     if(!mesg.has_value()) {
            //         return tl::make_unexpected(mesg.error());
            //     }

            //     handler(&mesg.value());
            // }


            return {};
        };
    }

    constexpr void set_raw_handler(uint32_t type, const std::function<tl::expected<void, NetworkError>(std::span<std::byte>)> handler) {
        m_handlers[type] = handler;
    }

    void update() {
        m_quicr_endpoint->poll();
        while(true) {
            std::vector<std::byte> buffer(64 * 1024);
            auto read_r = m_quicr_connection->read_into(buffer);

            if(!read_r) {
                spdlog::error("Failed to read from QUICr stream: {}", read_r.error().message());
                break;
            }

            if(*read_r == 0) {
                break;
            }

            uint32_t type = reinterpret_cast<uint32_t*>(buffer.data())[0];
            if(m_handlers[type] == nullptr) {
                spdlog::warn("Unknown message type: {}", type);
                throw std::runtime_error("Unknown message type: {}");
                break;
            }

            auto handler_r = m_handlers[type](std::span<std::byte>(buffer.data(), *read_r).subspan(sizeof(uint32_t)));
            if(!handler_r) {
                spdlog::error("Handler error");
                break;
            }
        }
        // while(m_server_messenger.peek().has_value() && m_server_messenger.peek().value().has_value()) {
        //     std::optional<PacketType> type = m_server_messenger.peek().value();
        //     if(type >= m_handlers.size() || m_handlers[type.value()] == nullptr) {
        //         spdlog::warn("Unknown message type: {}", (int)type.value());
        //         break;
        //     }

        //     auto r = m_handlers[type.value()]();
        //     if(!r) {
        //         spdlog::error("Failed to handle message: {}", r.error().message());
        //     }
        // }
    }

    template<std::derived_from<google::protobuf::Message> T>
    tl::expected<size_t, NetworkError> send(T& mesg) {
        std::string payload;
        if(!mesg.SerializeToString(&payload)) {
            spdlog::error("Failed to serialize message");
            return 0;
        }

        int32_t length = payload.length();
        if(length == 0) {
            return 0;
        }

        std::vector<std::byte> bytes(length + sizeof(uint32_t));

        uint32_t type = Message<T>::value;
        auto payload_bytes = std::as_writable_bytes(std::span(payload));
        memcpy(bytes.data(), &type, sizeof(type));
        memcpy(bytes.data() + sizeof(uint32_t), payload_bytes.data(), payload_bytes.size());

        auto send_r = m_quicr_connection->send_message(bytes, false);
        if(!send_r) {
            spdlog::error("Failed to send message: {}", send_r.error().message());
            return 0;
        }
        return payload_bytes.size();
    }
};

}
