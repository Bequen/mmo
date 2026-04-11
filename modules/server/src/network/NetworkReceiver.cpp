#include "NetworkReceiver.hpp"
#include "TcpListener.hpp"
#include "bytebuffer/ByteBufferStreamReader.hpp"
#include "frames/FrameCodec.hpp"
#include "network/InboundMessage.hpp"
#include "network/MessageQueue.hpp"
#include "network/OutboundMessage.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"

#include <stdexcept>

namespace tw::net {

TcpListener create_listener(int32_t port) {
    auto r = TcpListener::listen({{}, port}, port);
    if(!r) {
        spdlog::error("Failed to create TCP listener: {}", r.error().mesg());
        throw std::runtime_error("Failed to create TCP listener");
    }

    return std::move(*r);
}

NetworkReceiver::NetworkReceiver(
    PlayerSessionRegistry* session_registry,
    int32_t tcp_port,
    int32_t udp_port
) :
    m_session_registry(session_registry),
    m_inbound_queue(new MessageQueue<InboundMessage*>),
    // m_frame_router(inbound_message),
    m_tcp_listener(std::move(create_listener(tcp_port))),
    m_quicr_endpoint(std::make_unique<quicr::QuicrEndpoint>(quicr::QuicrEndpoint::create().value())),
    m_quicr_listener(std::make_unique<quicr::QuicrConnectionListener>(quicr::QuicrConnectionListener::listen(m_quicr_endpoint.get()).value()))

    // m_quicr_listener(std::move(quicr::QuicrConnectionListener::listen({{}, udp_port}).value()))
{
    auto bind_r = m_quicr_endpoint->bind(udp_port);
    m_quicr_endpoint->assign_listener(m_quicr_listener.get());

    spdlog::info("Running on port: {}", udp_port);
    auto non_blocking_r = m_tcp_listener.set_non_blocking();
    if(!non_blocking_r) {
        spdlog::error("Failed to set non-blocking mode for TCP listener");
        throw std::runtime_error("Failed to set non-blocking mode for TCP listener");
    }
}

bool NetworkReceiver::listen_tcp() {
    auto client = m_tcp_listener.listen();

    if(!client.has_value()) {
        return false;
    }

    if(!client.value().set_non_blocking().has_value()) {
        spdlog::error("Failed to set non-blocking mode for client");
        return false;
    }

    // m_session_registry->register_session(std::move(client.value()));

    spdlog::info("Client tries to connect");

    return true;
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

        auto data = std::vector<std::byte>(buffer.begin(), buffer.begin() + *read_r);
        m_inbound_queue->push(new InboundMessage(client->session_id, data));

        // size_t r = ByteBufferStreamReader::read(&client->tcp_stream, &client->inbound_buffer);
        // spdlog::info("Reading: {}", r);
        // if(r == 0) {
        //     continue;
        // }

        // auto frames = frame::FrameCodec::decode(client->inbound_buffer);

        // if(!frames.empty()) {
        //     m_frame_router.route(client->session_id, frames);
        // }
    }
}

void NetworkReceiver::update() {
    m_quicr_endpoint->poll();

    listen();

    process_streams();
}

}
