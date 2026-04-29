#include "ZoneServer.hpp"

#include "Cluster.pb.h"
#include "monitoring/NullMetricsReporter.hpp"
#include "monitoring/TimescaleDbMetricsReporter.hpp"
#include "runtime/LockStep.hpp"

#include <csignal>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

namespace tw::net {

static std::unique_ptr<NetworkMetricsReporter> make_reporter(const ZoneServerConfiguration& config) {
    if (config.timescaledb)
        return std::make_unique<TimescaleDbMetricsReporter>(*config.timescaledb);
    return std::make_unique<NullMetricsReporter>();
}

ZoneServer::ZoneServer(ZoneServerConfiguration config)
:
    m_metrics_reporter(make_reporter(config)),
    m_player_session_registry(std::make_unique<PlayerSessionRegistry>()),
    m_network_receiver(std::make_unique<NetworkReceiver>(
        m_player_session_registry.get(), config.quicr_port, m_metrics_reporter.get()
    )),
    m_message_dispatcher(std::make_unique<MessageDispatcher>(m_network_receiver.get())),
    m_replicator(std::make_unique<StateReplicator<SpatialBackendType>>(
        m_player_session_registry.get(),
        m_network_receiver.get()
    )),
    m_cluster_link(config.cluster_port),
    m_coordinator("127.0.0.1", config.cluster_port)
{
    auto [assigned_id, area_bounds, peers] = m_coordinator.register_zone();
    m_own_zone_id = assigned_id;

    m_zones.push_back(std::make_unique<ZoneManager>(area_bounds));

    spdlog::info("Registered as zone {} ({},{}) ({},{})",
        assigned_id,
        area_bounds.min.x, area_bounds.min.y,
        area_bounds.max.x, area_bounds.max.y);

    mmo::cluster::ZoneHello hello;
    hello.mutable_zone()->set_id(assigned_id);
    hello.mutable_zone()->mutable_bounds()->set_min_x(area_bounds.min.x);
    hello.mutable_zone()->mutable_bounds()->set_min_z(area_bounds.min.y);
    hello.mutable_zone()->mutable_bounds()->set_max_x(area_bounds.max.x);
    hello.mutable_zone()->mutable_bounds()->set_max_z(area_bounds.max.y);

    for (const auto& peer : peers) {
        spdlog::info("Connecting to peer zone {} at {}:{}", peer.id, peer.host, peer.port);
        auto* conn = m_cluster_link.connect_to_peer(peer.host, peer.port);
        if (conn)
            m_cluster_link.send_mesg(conn, hello);
    }
}

void ZoneServer::player_update_handler(SessionId session_id, mmo::PlayerMoveMessage&& message) {
    auto it = m_session_zone.find(session_id);
    if (it == m_session_zone.end()) return;
    it->second->on_player_move(session_id, std::move(message));
}

static std::atomic<bool> quit(false);

static void got_signal(int) {
    quit.store(true);
}

static void register_signal_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}

void ZoneServer::update_clients(uint32_t frame_idx) {
    m_network_receiver->update();
    m_message_dispatcher->drain_queue();

    while (m_network_receiver->peek_new_session()) {
        auto session_id = m_network_receiver->pop_new_session();

        glm::vec3 position = glm::vec3(
            (float)std::rand() / RAND_MAX * 20.0f,
            10.0f,
            (float)std::rand() / RAND_MAX * 20.0f
        );

        ZoneManager* zone = m_zones.front().get();
        entt::entity entity = zone->spawn_entity({ .name = "Pepik", .position = position });
        zone->add_client(session_id, entity);
        m_session_zone[session_id] = zone;

        spdlog::info("Client {} connected", session_id);
    }
}

void ZoneServer::run() {
    LockStep lock_step(20);
    register_signal_handler();

    uint32_t frame_idx = 1;

    m_message_dispatcher->set_handler<mmo::PlayerMoveMessage>(
        [&](uint64_t session_id, mmo::PlayerMoveMessage mesg) {
            player_update_handler(static_cast<SessionId>(session_id), std::move(mesg));
        });

    m_message_dispatcher->set_handler<mmo::chat::SendChatMessageRequest>(
        [&](uint64_t session_id, mmo::chat::SendChatMessageRequest mesg) {
        });

    while (!quit.load()) {
        if (lock_step.wait_for_next_step()) { continue; }

        m_cluster_link.update();

        FrameMarkStart("Update clients");
        update_clients(frame_idx);
        FrameMarkEnd("Update clients");

        for (auto& zone : m_zones) {
            zone->tick(frame_idx, lock_step.delta_time());

            m_replicator->replicate(zone->registry(), zone->interest());
        }

        m_network_receiver->update();

        m_metrics_reporter->set_player_count(m_session_zone.size());
        m_metrics_reporter->tick();

        frame_idx++;
        FrameMark;
    }

    // Notify all peers that this zone server is going away.
    for (auto& zone : m_zones) {
        mmo::cluster::ZoneBye bye;
        bye.mutable_zone()->set_id(m_own_zone_id);
        bye.mutable_zone()->mutable_bounds()->set_min_x(zone->area().min.x);
        bye.mutable_zone()->mutable_bounds()->set_min_z(zone->area().min.y);
        bye.mutable_zone()->mutable_bounds()->set_max_x(zone->area().max.x);
        bye.mutable_zone()->mutable_bounds()->set_max_z(zone->area().max.y);
        m_cluster_link.broadcast(bye);
    }
}

} // namespace tw::net
