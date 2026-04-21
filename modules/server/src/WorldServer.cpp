#include "WorldServer.hpp"


#include "Chat.pb.h"
#include "MessageRegistry.hpp"
#include "PlayerClient.hpp"
#include "network/OutboundMessage.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "runtime/LockStep.hpp"
#include "world/CharacterController.hpp"
#include "world/WorldEntity.hpp"
#include "world/CharacterBody.hpp"

#include <csignal>
#include <cstdlib>
#include <tracy/Tracy.hpp>


namespace tw::net {

WorldServer::WorldServer(int port, int quicr_port) :
    m_monitor(std::make_unique<TimescaleDbBandwidthMonitor>((PostgreSqlConnectionInfo){
        .host = "localhost",
        .port = 5432,
        .dbname = "towards",
        .username = "postgres",
        .password = "postgres"
    })),
    // m_client_manager{port, quicr_port, m_monitor.get()},
    m_player_session_registry(std::make_unique<PlayerSessionRegistry>()),
    m_message_dispatcher(std::make_unique<MessageDispatcher>(m_network_receiver.get())),
    m_network_receiver(std::make_unique<NetworkReceiver>(m_player_session_registry.get(), port, quicr_port)),
    m_world(std::make_unique<World>()),
    m_physics_world(m_world.get()),
    m_interest_system(std::make_unique<im::InterestSystem<SpatialBackendType>>(
        m_world.get(), m_player_session_registry.get()
    )),
    m_replicator(std::make_unique<StateReplicator<SpatialBackendType>>(
        m_world.get(), m_player_session_registry.get(), m_network_receiver.get(), m_interest_system.get()
    ))
{ }

void WorldServer::player_update_handler(PlayerClientId clientId, mmo::PlayerMoveMessage&& message) {
    ZoneScoped;
    entt::entity entity = get_client_entity(clientId);

    auto body = m_world->registry()
                        .try_get<CharacterController>(entity);

    // m_client_manager.set_frame_idx(clientId, message.frame_idx());

    if(body) {
        body->set_input(message.frame_idx(), glm::vec3(message.input().x(), message.input().y(), message.input().z()));
    } else {
        spdlog::error("Player {} has no character controller", (uint32_t)entity);
    }
}

std::atomic<bool> quit(false);    // signal flag

void got_signal(int) {
    quit.store(true);
}

void register_signal_handler() {
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);
}

void WorldServer::update_clients(uint32_t frame_idx) {
    m_network_receiver->update();

    m_message_dispatcher->drain_queue();

    while(m_network_receiver->peek_new_session()) {
        auto session_id = m_network_receiver->pop_new_session();

        glm::vec3 position = glm::vec3((float)std::rand() / RAND_MAX * 20.0f, 10.0f, (float)std::rand() / RAND_MAX * 20.0f);

        auto entity = spawn_entity((entt::entity)0, {
            .name = std::string("Pepik"),
            .position = position
        });

        m_client_entity_mappings[session_id] = entity;
        m_interest_system->set_interest_delegate(session_id, entity);
        spdlog::info("Client {} connected", session_id);
    }
}

void WorldServer::run() {
    LockStep lock_step(20);

    register_signal_handler();

    uint32_t frame_idx = 1;

    m_message_dispatcher->set_handler<mmo::PlayerMoveMessage>(
        [&](PlayerClientId clientId, mmo::PlayerMoveMessage mesg) {
            player_update_handler(clientId, std::move(mesg));
        });

    m_message_dispatcher->set_handler<mmo::chat::SendChatMessageRequest>(
        [&](PlayerClientId clientId, mmo::chat::SendChatMessageRequest mesg) {
        });

    while(!quit.load()) {
        if(lock_step.wait_for_next_step()) { continue; }

        FrameMarkStart("Update clients");
        update_clients(frame_idx);
        FrameMarkEnd("Update clients");

        FrameMarkStart("Interest System");
        m_interest_system->update();
        FrameMarkEnd("Interest System");

        FrameMarkStart("Replicator");
        m_replicator->replicate();
        FrameMarkEnd("Replicator");

        FrameMarkStart("World step");
        m_world->step(lock_step.delta_time());
        FrameMarkEnd("World step");

        FrameMarkStart("Physics step");
        m_physics_world.step(frame_idx, lock_step.delta_time());
        FrameMarkEnd("Physics step");

        m_network_receiver->update();

        frame_idx++;
        FrameMark;
    }
}

entt::entity WorldServer::spawn_entity(entt::entity entity, EntityInfo&& info) {
    entity = m_world->registry().create();

    m_world->registry().emplace<WorldEntity>(entity, info.name, (uint32_t)entity);
    m_world->registry().emplace<Transform>(entity, info.position);
    m_world->registry().emplace<CharacterController>(entity, 20.0f);
    m_world->registry().emplace<CharacterBody>(entity, m_physics_world.create_character(
                new JPH::BoxShape(JPH::Vec3Arg(0.5f, 0.5f, 0.5f)),
                info.position));

    return entity;
}

}
