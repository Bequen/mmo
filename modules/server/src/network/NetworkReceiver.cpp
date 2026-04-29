#include "NetworkReceiver.hpp"
#include "network/InboundMessage.hpp"
#include "network/MessageQueue.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"

namespace tw::net {

NetworkReceiver::NetworkReceiver(
    PlayerSessionRegistry* session_registry,
    int32_t udp_port,
    NetworkMetricsReporter* metrics_reporter
) :
    m_session_registry(session_registry),
    m_inbound_queue(new MessageQueue<InboundMessage*>),
    m_quicr_endpoint(std::make_unique<quicr::QuicrEndpoint>(quicr::QuicrEndpoint::create().value())),
    m_quicr_listener(std::make_unique<quicr::QuicrConnectionListener>(quicr::QuicrConnectionListener::listen(m_quicr_endpoint.get()).value())),
    m_metrics_reporter(metrics_reporter)
{
    m_quicr_endpoint->bind(udp_port);
    m_quicr_endpoint->assign_listener(m_quicr_listener.get());

    spdlog::info("Running on port: {}", udp_port);
}

bool NetworkReceiver::listen_quicr() {
    quicr::QuicrConnection* connection = nullptr;
    while((connection = m_quicr_listener->listen())) {
        spdlog::info("Quicr client tries to connect");

        auto session_id = m_session_registry->register_session(connection);

        m_new_sessions.push_back(session_id);
    }

    return true;
}

void NetworkReceiver::listen() {
    listen_quicr();
}

void NetworkReceiver::process_streams() {
    m_quicr_endpoint->poll();

    std::vector<std::byte> buffer(64 * 1024);
    for(auto& client : m_session_registry->sessions()) {
        auto read_r = client->quicr_connection->read_into(buffer);
        if(!read_r) {
            spdlog::error("Failed to read from QUICr stream");
            continue;
        }

        if(*read_r == 0) {
            continue;
        }

        m_metrics_reporter->add_inbound(*read_r);

        auto data = std::vector<std::byte>(buffer.begin(), buffer.begin() + *read_r);
        m_inbound_queue->push(new InboundMessage(client->session_id, data));
    }
}

void NetworkReceiver::update() {
    m_quicr_endpoint->poll();

    listen();

    process_streams();
}

}
