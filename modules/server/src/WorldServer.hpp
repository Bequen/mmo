#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "LoginHandler.hpp"
#include "TcpSocket.hpp"
#include "PlayerClient.hpp"
#include "entt/entity/fwd.hpp"
#include "interest_management/DistanceInterestManagement.hpp"
#include "messages/PlayerMoveMessage.hpp"
#include "messages/WorldStateMessage.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/World.hpp"

namespace tw::net {

struct EntityInfo {
    std::string name;
    glm::vec3 position;
};

class WorldServer {
private:
    TcpSocket m_server_socket;
    entt::registry m_registry;

    std::vector<PlayerClient> m_clients;

    World m_world;
    JoltPhysicsWorld m_physics_world;
    server::im::DistanceInterestManagement<EntityPosition> m_im;

    LoginHandler m_login_handler;

    void enqueue_new_connections();

    WorldStateMessage build_world_status_message();

    void handle_player_move(entt::entity entity, PlayerInputMessage&& message);

    void join_request_handler(PlayerClient& client, const PlayerJoinRequestMessage& request);

    std::vector<entt::entity> cleanup_clients();

    void update_clients(uint32_t frame_idx);

public:
    WorldServer(int port);

    WorldServer(TcpSocket&& socket);

    void run();

    entt::entity spawn_entity(entt::entity entity, EntityInfo&& info);
};

}
