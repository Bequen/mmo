#include "ZoneClusterLink.hpp"

#include "Address.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace tw::net {

ZoneClusterLink::ZoneClusterLink(int port)
    : m_endpoint(std::make_unique<quicr::QuicrEndpoint>(
          quicr::QuicrEndpoint::create().value()
      )),
      m_listener(std::make_unique<quicr::QuicrConnectionListener>(
          quicr::QuicrConnectionListener::listen(m_endpoint.get()).value()
      ))
{
    if (auto r = m_endpoint->bind(port); !r) {
        throw std::runtime_error("ZoneClusterLink: failed to bind to port " + std::to_string(port));
    }
    m_endpoint->assign_listener(m_listener.get());
    spdlog::info("ZoneClusterLink listening on port {}", port);
}

void ZoneClusterLink::update() {
    m_endpoint->poll();

    quicr::QuicrConnection* connection;
    while ((connection = m_listener->listen())) {
        spdlog::info("Zone peer connected from {}", connection->address().to_string());
        m_peers.push_back(connection);
    }
}

quicr::QuicrConnection* ZoneClusterLink::connect_to_peer(const std::string& host, int port) {
    auto result = m_endpoint->connect(Address(host, port));
    if (!result) {
        spdlog::error("ZoneClusterLink: failed to connect to peer {}:{}", host, port);
        return nullptr;
    }
    quicr::QuicrConnection* conn = result.value();
    m_peers.push_back(conn);
    spdlog::info("ZoneClusterLink: connected to peer {}:{}", host, port);
    return conn;
}

} // namespace tw::net
