#include "WorldServer.hpp"
#include "Address.hpp"
#include "MessageHandler.hpp"
#include "Messenger.hpp"
#include "PlayerClient.hpp"
#include "TcpSocket.hpp"
#include "entt/entity/fwd.hpp"
#include "exception/SocketClosedException.hpp"
#include "messages/PlayerMoveMessage.hpp"
#include "messages/WorldStateMessage.hpp"
#include "packets/Packet.hpp"
#include "world/CharacterController.hpp"
#include "world/World.hpp"
#include "world/WorldEntity.hpp"
#include "runtime/LockStep.hpp"
#include "world/Transform.hpp"
#include "world/CharacterBody.hpp"
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <vector>


namespace tw::net {

WorldServer::WorldServer(int port) :
    m_im(&m_world, 10.0f),
    m_physics_world(&m_world)
{
    m_server_socket.bind(Address({}, port));
}

WorldServer::WorldServer(TcpSocket&& socket) :
    m_server_socket(std::move(socket)),
    m_physics_world(&m_world),
    m_im(&m_world, 10.0f) {
}

void WorldServer::enqueue_new_connections() {
    std::optional<TcpSocket> client = {};
    do {
        client = m_server_socket.listen();
        
        if(client.has_value()) {
            client.value().set_non_blocking();

            m_clients.emplace_back(0,
                    tw::net::Messenger(std::move(client.value())),
                    "Test");

            std::println("Client tries to connect as guest");
        } 
    } while(client.has_value());
}

WorldStateMessage WorldServer::build_world_status_message() {
    WorldStateMessage mesg = {};
    m_registry.view<tw::Transform, WorldEntity>()
        .each([&](const auto entity, tw::Transform& ts, WorldEntity& world_entity) {
                mesg.player_states.push_back({
                    .id = (uint32_t)entity,
                    .name = world_entity.name,
                    .position = ts.position()
                });
        });

    return mesg;
}

void WorldServer::handle_player_move(entt::entity entity, PlayerInputMessage&& message) {
    auto ts = m_registry.try_get<Transform>((entt::entity)message.entity_id);
    auto client = m_registry.try_get<PlayerClient>(entity);
    if(ts) {
        client->m_last_frame_idx = message.frame_idx;
    }
}

void WorldServer::join_request_handler(PlayerClient& client, const PlayerJoinRequestMessage& mesg) {
    std::println("Player {} joined!", mesg.name);
    client.m_name = mesg.name;

    glm::vec3 position = glm::vec3((float)std::rand() / RAND_MAX * 20.0f, 10.0f, (float)std::rand() / RAND_MAX * 20.0f);

    auto entity = spawn_entity((entt::entity)client.m_entity_id, {
        .name = mesg.name,
        .position = position 
    });

    client.m_entity_id = (uint32_t)entity;

    PlayerJoinResponseMessage response = {
        .is_valid = true,
        .entity_id = (uint32_t)entity,
        .position = position 
    };

    client.m_messenger.send(response);

    EntitySpawnMessage client_spawn_notification = {
        .entity_id = response.entity_id,
        .is_player = true,
        .name = mesg.name, 
        .position = response.position 
    };

    for(auto& other_client : m_clients) {
        if(client.m_entity_id == other_client.m_entity_id) {
            return;
        }

        auto ts = m_world.registry().try_get<Transform>((entt::entity)other_client.m_entity_id);

        if(ts) {
            EntitySpawnMessage spawn_notification = {
                .entity_id = other_client.m_entity_id,
                .is_player = true,
                .name = other_client.m_name,
                .position = ts->position()
            };

            client.m_messenger.send(spawn_notification);
            other_client.m_messenger.send(client_spawn_notification);
        }
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

std::vector<entt::entity> WorldServer::cleanup_clients() {
    std::vector<entt::entity> entities;
    
    m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(),
            [&](PlayerClient& client) {
                if(client.m_messenger.is_closed()) {
                    std::println("Client {} disconnected", client.m_name);
                    m_world.registry().destroy((entt::entity)client.m_entity_id);
                    entities.push_back((entt::entity)client.m_entity_id);
                    return true;
                }
                return false;
            }), m_clients.end());

    return entities;
}

void WorldServer::update_clients(uint32_t frame_idx) {
    auto disconnected_clients = cleanup_clients();

    for(auto& client : m_clients) {
        try {
            client.m_messenger.fetch();

            while(client.m_messenger.peek()) {
                switch(client.m_messenger.peek().value()) {
                    case PLAYER_JOIN_REQUEST_PACKET:
                        join_request_handler(client,
                                client.m_messenger.pop<PlayerJoinRequestMessage>());
                        break;
                    case PLAYER_MOVE_PACKET: {
                        auto mesg = client.m_messenger.pop<PlayerInputMessage>();

                        auto body = m_world.registry()
                            .try_get<CharacterController>((entt::entity)client.m_entity_id);

                        client.set_last_frame_idx(mesg.frame_idx);

                        if(body) body->set_input(frame_idx, mesg.inputs);
                    } break;
                    default: // clear unknown messages
                        client.m_messenger.clear();
                }
            }

            // TODO: Put to queue
            for(auto entity : disconnected_clients) {
                EntityDespawnMessage despawn_message = {
                    .entity_id = (uint32_t)entity,
                    .is_player = true
                };

                client.m_messenger.send(despawn_message);
            }

            if(!client.m_frame_responded) {
                auto interest = m_im.query((entt::entity)client.m_entity_id).m_entities;
                EntityMoveMessage move_message = {
                    .frame_idx = client.m_last_frame_idx,
                    .entities = std::vector<EntityPosition>(10)
                };

                for(int i = 0; i < interest.size(); i += 10) {
                    int o = 0;
                    for(o = i; o < std::min((uint32_t)interest.size(), (uint32_t)(i + 1) * 10); o++) {
                        move_message.entities[o - i] = interest[o];
                    }
                    move_message.entities.resize(o - i);
                    if(o - i > 0) {
                        client.m_messenger.send(move_message);
                    }
                }

                client.m_frame_responded = true;
            }
        } catch(std::exception& e) {
            std::println("Exception: {}", e.what());
        }
    }
}

void WorldServer::run() {
    LockStep lock_step(20);

    register_signal_handler();

    uint32_t frame_idx = 1;

    while(!quit.load()) {
        if(lock_step.wait_for_next_step()) { continue; }

        enqueue_new_connections();

        update_clients(frame_idx);

        m_world.step(lock_step.delta_time());
        m_physics_world.step(frame_idx, lock_step.delta_time());


        m_world.registry().view<Transform, CharacterBody>()
            .each([&](Transform& ts, CharacterBody& rb) {
                    auto character = rb.m_character;

                    auto transform = character->GetWorldTransform();
                    memcpy(&ts.transform, &transform, sizeof(glm::mat4));
            });


        frame_idx++;
    }
}

entt::entity WorldServer::spawn_entity(entt::entity entity, EntityInfo&& info) {
    entity = m_world.registry().create();

    m_world.registry().emplace<WorldEntity>(entity, info.name, (uint32_t)entity);
    m_world.registry().emplace<Transform>(entity, info.position);
    m_world.registry().emplace<CharacterController>(entity, 20.0f);
    m_world.registry().emplace<CharacterBody>(entity, m_physics_world.create_character(
                new JPH::BoxShape(JPH::Vec3Arg(0.5f, 0.5f, 0.5f)), 
                info.position));

    return entity;
}

}
