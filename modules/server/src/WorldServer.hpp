#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <system_error>
#include <unordered_map>

#include "ClientManager.hpp"
#include "Login.pb.h"
#include "PlayerMove.pb.h"
#include "TcpListener.hpp"
#include "PlayerClient.hpp"
#include "entt/entity/fwd.hpp"
#include "interest_management/DistanceInterestManagement.hpp"
#include "interest_management/FixedGrid.hpp"
#include "messages/PlayerMoveMessage.hpp"
#include "monitoring/BandwidthMonitor.hpp"
#include "network/MessageDispatcher.hpp"
#include "network/OutboundMessage.hpp"
#include "network/InboundMessage.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "replication/StateReplicator.hpp"
#include "systems/InterestSystem.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/World.hpp"
#include "network/NetworkReceiver.hpp"

namespace tw::net {

struct EntityInfo {
    std::string name;
    glm::vec3 position;
};

// Spatial backend: FixedGrid for a bounded world (10km × 10km, 500-unit cells)
// Adjust the bounds and cell size to match your world configuration.
// CELL_SIZE = VIEW_RADIUS (500) means a 3×3 grid neighborhood with no distance checks.
using SpatialBackendType = im::FixedGrid<-5000, 5000, -5000, 5000, 500, 500>;

class WorldServer {
private:
    std::unique_ptr<BandwidthMonitor> m_monitor;
    // ClientManager m_client_manager;

    std::unique_ptr<PlayerSessionRegistry> m_player_session_registry;

    std::unique_ptr<NetworkReceiver> m_network_receiver;

    std::unique_ptr<MessageDispatcher> m_message_dispatcher;

    std::unique_ptr<World> m_world;
    JoltPhysicsWorld m_physics_world;

    std::unordered_map<PlayerClientId, entt::entity> m_client_entity_mappings;

    std::unique_ptr<im::InterestSystem<SpatialBackendType>> m_interest_system;
    std::unique_ptr<StateReplicator<SpatialBackendType>> m_replicator;

    inline entt::entity get_client_entity(PlayerClientId clientId) {
        return m_client_entity_mappings[clientId];
    }

    void player_update_handler(PlayerClientId clientId, mmo::PlayerMoveMessage&& message);

    void notify_new_entity(PlayerClient& client);

    void update_clients(uint32_t frame_idx);

public:
    WorldServer(int port, int quicr_port);

    void run();

    entt::entity spawn_entity(entt::entity entity, EntityInfo&& info);
};

}
