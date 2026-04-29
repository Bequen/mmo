#pragma once

#include "MessageRegistry.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"

#include <memory>
#include <span>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace tw::net {

// Listens on a dedicated QUICr port for incoming zone-server peer connections
// and opens outgoing connections to known peers.
//
// All connections (incoming and outgoing) are collected in m_peers so that
// broadcast messages (ZoneHello, ZoneBye) reach every peer uniformly.
//
// Accepted QuicrConnection* pointers are owned by the endpoint and remain valid
// for the lifetime of this object.
class ZoneClusterLink {
    std::unique_ptr<quicr::QuicrEndpoint>           m_endpoint;
    std::unique_ptr<quicr::QuicrConnectionListener> m_listener;
    std::vector<quicr::QuicrConnection*>            m_peers;

public:
    explicit ZoneClusterLink(int port);

    // Poll for datagrams and accept any pending peer connections.
    void update();

    // Connect to a remote zone peer and register it in the peer list.
    quicr::QuicrConnection* connect_to_peer(const std::string& host, int port);

    // Serialize and send a protobuf message to a single peer.
    template<typename T>
    void send_mesg(quicr::QuicrConnection* conn, const T& msg) {
        std::string payload;
        if (!msg.SerializeToString(&payload)) {
            spdlog::error("ZoneClusterLink: failed to serialize message");
            return;
        }
        std::vector<std::byte> bytes(payload.size() + sizeof(uint32_t));
        uint32_t type = tw::Message<T>::value;
        memcpy(bytes.data(), &type, sizeof(type));
        memcpy(bytes.data() + sizeof(uint32_t), payload.data(), payload.size());
        conn->send_message(bytes, false);
    }

    // Broadcast a protobuf message to all connected peers.
    template<typename T>
    void broadcast(const T& msg) {
        for (auto* peer : m_peers)
            send_mesg(peer, msg);
    }

    std::span<quicr::QuicrConnection* const> peers() const { return m_peers; }
};

} // namespace tw::net
