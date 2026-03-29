#pragma once

#include "Login.pb.h"
#include "TcpListener.hpp"
#include "UdpStream.hpp"
#include "messenger/Messenger.hpp"
#include "monitoring/BandwidthMonitor.hpp"
#include "monitoring/TimescaleDbBandwidthMonitor.hpp"
#include "packets/Packet.hpp"
#include "packets/LoginPacket.hpp"
#include "PlayerClient.hpp"
#include "tl/expected.hpp"
#include <cerrno>
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <pqxx/pqxx>
#include <tracy/Tracy.hpp>

namespace tw::net {


struct PendingConnection {
  uint32_t consecutive_failures;
  Messenger<std::byte, TcpStream> messenger;

  PendingConnection(Messenger<std::byte, TcpStream>&& messenger) :
    consecutive_failures(0),
    messenger(std::move(messenger))
  { }
};

/**
 * Manages client connections. Ignores connections that are closed.
 */
class ClientManager {
private:
    const uint32_t MAX_MESSAGES_PER_CLIENT_PER_FRAME = 10;

    std::unique_ptr<tw::net::TcpListener> m_listener;

    std::vector<std::byte> m_udp_input_buffer;
    std::unique_ptr<UdpStream> m_udp_listener;

    std::vector<std::unique_ptr<PendingConnection>> m_pending_connections;
    std::unordered_map<PlayerClientId, std::unique_ptr<PlayerClient>> m_clients;
    uint32_t m_client_id_counter;

    std::unordered_map<uint32_t, std::function<tl::expected<void, NetworkError>(PlayerClientId, std::unique_ptr<PlayerClient>&)>> m_handlers;

    std::vector<PlayerClientId> m_joined_buffer;
    std::vector<PlayerClientId> m_disconnected_buffer;

    BandwidthMonitor* m_monitor;

    void process_datagram(std::span<std::byte> dgram) {
        uint32_t client_id = *reinterpret_cast<uint32_t*>(dgram.data());
        if(m_clients.find(client_id) == m_clients.end()) {
            spdlog::warn("Received datagram for unknown client id: {}", client_id);
            return;
        }

        auto client = m_clients.find(client_id);
    }

    void listen() {
        tl::expected<TcpStream, NetworkResult> client = tl::make_unexpected(NetworkResult::ok());

        do {
            auto client = m_listener->listen();

            if(!client.has_value()) {
                break;
            }

            if(!client.value().set_non_blocking().has_value()) {
                spdlog::error("Failed to set non-blocking mode for client");
                break;
            }

            // m_pending_connections.emplace(client-> std::make_unique<PendingConnection>(std::move(client.value())));

            spdlog::info("Client tries to connect");
        } while(true);

        do {
            auto r = m_udp_listener->read_into(std::span(m_udp_input_buffer));

            if(!r || *r == 0) {
                break;
            }

            process_datagram(std::span(m_udp_input_buffer.data(), r.value()));
        } while(true);
    }

    void handle_pending_connections() {
        if(m_pending_connections.empty()) {
            return;
        }

        std::erase_if(m_pending_connections, [&](std::unique_ptr<PendingConnection>& pending) {
            auto peeked = pending->messenger.peek();
            if(!peeked.has_value()) {
                return false;
            }

            if(peeked == (int32_t)Message<mmo::LoginRequest>::value) {
                auto login_request = pending->messenger.pop<mmo::LoginRequest>(nullptr);
                uint32_t client_id = m_client_id_counter++;

                mmo::LoginResponse login_response = {};
                login_response.set_success(true);
                login_response.set_message("prd");
                login_response.set_entity_id((int)client_id);

                auto result = pending->messenger.send(login_response);

                if(!result.has_value()) {
                    pending->consecutive_failures++;
                    return pending->consecutive_failures >= 3;
                }

                m_clients.emplace(client_id, std::make_unique<PlayerClient>(
                    client_id, "test",
                    std::move(pending->messenger)
                ));
                m_joined_buffer.push_back(client_id);

                spdlog::info("Client tries to login");

                return true;
            }

            return false;
        });
    }


public:
    GET_REF(m_clients, clients);

    ClientManager(
        int port,
        int quicr_port,
        BandwidthMonitor* monitor
    ) :
        m_listener(),
        m_udp_input_buffer(64 * 1024),
        m_clients(10),
        m_monitor(monitor),
        m_pending_connections(),
        m_client_id_counter(0),
        m_handlers(),
        m_joined_buffer(),
        m_disconnected_buffer()
    {
        m_listener->set_non_blocking();
        spdlog::info("Starting world server on port: {}", m_listener->address().port());
       //  set_join_handlers();
    }

    void set_frame_idx(PlayerClientId id, uint32_t frame_idx) {
        m_clients[id]->set_last_frame_idx(frame_idx);
    }

    template<typename T>
    void set_handler(std::function<void(PlayerClientId, T)> handler) {
        m_handlers[static_cast<uint32_t>(Message<T>::value)] =
            [handler, this](uint32_t id, std::unique_ptr<PlayerClient>& messenger) -> tl::expected<void, NetworkError> {
                ZoneScopedN("Handling message");

                size_t size = 0;
                auto message = messenger->messenger().pop<T>(&size);
                if(!message.has_value()) {
                    return tl::make_unexpected(message.error());
                }

                m_monitor->add_inbound(size);

                handler(id, message.value());
                return {};
            };
    }

    /**
     * Pops new clients that joined. Won't return the same clients after.
     * @returns Vector of new clients that joined.
     */
    std::vector<PlayerClientId> pop_connected_clients() {
        std::vector<PlayerClientId> joined_clients(m_joined_buffer);
        m_joined_buffer.clear();
        return joined_clients;
    }

    /**
     * Pops clients that disconnected.
     * @returns Vector of clients that disconnected.
     */
    std::vector<PlayerClientId> pop_disconnected_clients() {
        std::vector<PlayerClientId> disconnected_clients(m_disconnected_buffer);
        m_disconnected_buffer.clear();
        return disconnected_clients;
    }

    template<typename T>
    bool send(PlayerClientId clientId, T message) {
        if(m_clients.find(clientId) == m_clients.end()) {
            spdlog::warn("Attempted to send message to client that is disconnected.");
            return false;
        }

        auto r = m_clients[clientId]->messenger().send(message);

        if(!r.has_value()) {
            spdlog::error("Failed to send message: {}", r.error().message());
            m_clients[clientId]->add_failure();
            return false;
        }

        m_monitor->add_outbound(r.value());

        return true;
    }

    template<typename T>
    bool send_fast(PlayerClientId clientId, T message) {
        if(m_clients.find(clientId) == m_clients.end()) {
            spdlog::warn("Attempted to send message to client that is disconnected.");
            return false;
        }

        auto& client = m_clients[clientId];
        size_t sent = 0;

        auto r = client->messenger().send(message);
        if(!r.has_value()) {
            spdlog::error("Failed to send message: {}", r.error().message());
            m_clients[clientId]->add_failure();
            return false;
        }

        sent = r.value();

        m_monitor->add_outbound(sent);

        return true;
    }

    void update() {
        listen();

        handle_pending_connections();

        for (auto& client_pair : m_clients) {
            ZoneScopedN("Handling client messages");

            auto& client = client_pair.second;
            for(int i = 0; i < MAX_MESSAGES_PER_CLIENT_PER_FRAME; i++) {
                ZoneScopedN("Handling message")
                auto peek_result = client->messenger().peek();

                if(!peek_result.has_value()) {
                    spdlog::error("Failed to peek message: {}", peek_result.error().message());
                    client->add_failure();
                    break;
                }

                if(!peek_result.value().has_value()) {
                    break;
                }

                auto peek = peek_result.value().value();
                client->set_received();

                if(!m_handlers.contains(peek)) {
                    spdlog::error("No handler for message {}", (uint32_t)peek);
                    client->messenger().skip();
                    break;
                }

                auto handler_result = m_handlers[peek](client->id(), client);

                if(!handler_result.has_value()) {
                    spdlog::error("Handler failed for message {}", (uint32_t)peek);
                    client->add_failure();
                    client->messenger().skip();
                    break;
                }
            }
        }

        std::erase_if(m_clients, [&](auto& client) {
            const auto& [key, value] = client;
            if(value->is_disconnected()) {
                spdlog::error("Client disconnected");
                m_disconnected_buffer.push_back(key);
                return true;
            }

            return false;
        });
    }
};

}
