#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "PlayerMove.pb.h"
#include "ZoneClusterLink.hpp"
#include "ZoneCoordinator.hpp"
#include "ZoneManager.hpp"
#include "ZoneServerConfiguration.hpp"
#include "monitoring/MetricsReporter.hpp"
#include "network/MessageDispatcher.hpp"
#include "network/NetworkReceiver.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "replication/StateReplicator.hpp"

namespace tw::net {

class ZoneServer {
    std::unique_ptr<NetworkMetricsReporter>              m_metrics_reporter;
    std::unique_ptr<PlayerSessionRegistry>               m_player_session_registry;
    std::unique_ptr<NetworkReceiver>                     m_network_receiver;
    std::unique_ptr<MessageDispatcher>                   m_message_dispatcher;
    std::unique_ptr<StateReplicator<SpatialBackendType>> m_replicator;
    ZoneClusterLink                                      m_cluster_link;
    ZoneCoordinator                                      m_coordinator;

    std::vector<std::unique_ptr<ZoneManager>>   m_zones;
    std::unordered_map<SessionId, ZoneManager*> m_session_zone;
    uint32_t                                    m_own_zone_id = 0;

    void player_update_handler(SessionId session_id, mmo::PlayerMoveMessage&& message);
    void update_clients(uint32_t frame_idx);

public:
    explicit ZoneServer(ZoneServerConfiguration config);
    void run();
};

} // namespace tw::net
