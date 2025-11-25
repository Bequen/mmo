#include "ClientWorldController.hpp"

#include <glm/glm.hpp>

#include "Address.hpp"
#include "MessageHandler.hpp"
#include "Messenger.hpp"
#include "TcpSocket.hpp"
#include "imgui.h"
#include "messages/PlayerMoveMessage.hpp"
#include "world/CharacterBody.hpp"
#include "world/CharacterController.hpp"
#include "world/Interpolation.hpp"
#include "world/EntityInterpolation.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/WorldEntity.hpp"
#include <chrono>

namespace tw {


entt::entity create_player_entity(World* world, JoltPhysicsWorld* physics_world, drw::WorldRenderer* renderer) {
    entt::entity entity = world->registry().create();

    drw::Mesh mesh = renderer->add_mesh(drw::MeshData::cube(glm::vec3(1.0f)));

    world->registry()
        .emplace<Transform>(entity, 
                Transform(glm::vec3(0.0f, 10.0f, 0.0f)));

    world->registry()
        .emplace<tw::WorldEntity>(entity, 
                tw::WorldEntity(std::string("player"), (uint32_t)entity));

    world->registry()
        .emplace<tw::drw::Mesh>(entity, mesh);

    world->registry()
        .emplace<EntityInterpolation>(entity, glm::vec3(0.0f, 10.0f, 0.0f));

    world->registry()
        .emplace<tw::CharacterController>(entity, 20.0f);
    auto* body = &world->registry()
        .emplace<CharacterBody>(entity, physics_world->create_character(
                new JPH::BoxShape(JPH::Vec3Arg(0.5f, 0.5f, 0.5f)), 
                glm::vec3(0.0f, 10.0f, 0.0f)));


    return entity;
}


entt::entity 
ClientWorldController::create_entity(const std::string& name, glm::vec3 position) {
    const auto entity = m_world->registry().create();

    if(!m_mesh.has_value()) {
        m_mesh = m_world_renderer->add_mesh(drw::MeshData::cube(glm::vec3(1.0f)));
    }
    std::println("Creating entity {}", name);

    m_world->registry()
        .emplace<Transform>(entity, 
                Transform(position));

    m_world->registry()
        .emplace<tw::WorldEntity>(entity, 
                tw::WorldEntity(name, (uint32_t)entity));

    m_world->registry()
        .emplace<EntityInterpolation>(entity, position);

    m_world->registry()
        .emplace<tw::drw::Mesh>(entity, 
                m_mesh.value());
    
    return entity;
}

net::MessageHandler create_messenger(tw::net::Address address) {
    net::TcpSocket socket;
    socket.connect(tw::net::Address("127.0.0.1", 8100));
    socket.set_non_blocking();

    return net::MessageHandler(net::Messenger(std::move(socket)));
}

ClientWorldController::ClientWorldController(
        const io::InputManager* inputs,
        World* world,
        JoltPhysicsWorld* physics_world,
        drw::WorldRenderer* world_renderer,
        tw::net::Address address
) :
    m_input_manager(inputs), 
    m_world(world),
    m_physics_world(physics_world),
    m_world_renderer(world_renderer),
    m_player_entity(create_player_entity(world, physics_world, world_renderer)),
    m_player_controller(&world_renderer->camera(), glm::vec3()),
    m_messenger(create_messenger(address)),
    m_tick_step(20),
    m_is_connected(false),
    m_position_history(0, glm::vec3())
{
    m_messenger->set_handler<EntitySpawnMessage>(
        [&](EntitySpawnMessage* mesg) {
            std::println("Player connected");
            m_entity_mapping[mesg->entity_id] = create_entity(mesg->name, mesg->position);
        });
    
    m_messenger->set_handler<EntityDespawnMessage>(
        [&](EntityDespawnMessage* mesg) {
            std::println("Player disconnected");
            m_world->registry().destroy((entt::entity)m_entity_mapping[mesg->entity_id]);
            m_entity_mapping.erase(mesg->entity_id);
        });

    m_messenger->set_handler<EntityMoveMessage>(
        [&](EntityMoveMessage* mesg) {
            for(auto& e : mesg->entities) {
                auto entity = m_entity_mapping[e.entity_id];

                if(entity == m_player_entity) {
                    auto position_history = m_position_history.get(mesg->frame_idx);
                    if(!position_history.has_value()) {
                        continue;
                    }

                    glm::vec3 original = *position_history.value();
                    glm::vec3 server = e.position;

                    if(original.x != server.x || original.y != server.y || original.z != server.z) {
                        auto* character = m_world->registry().try_get<CharacterController>(entity);
                        std::println("FrameIdx: {}", mesg->frame_idx);
                        std::println("Original: {} {} {}", original.x, original.y, original.z);
                        std::println("Server: {} {} {}", server.x, server.y, server.z);
                //         m_physics_world->rollback(mesg->frame_idx);
                // 
                //         auto* ts = m_world->registry().try_get<Transform>(entity);
                //         ts->set_position(server);
                //
                //         m_physics_world->apply_rollback(mesg->frame_idx);
                    }
                } else {
                    auto* prop = m_world->registry().try_get<EntityInterpolation>(entity);
                    prop->push(std::chrono::steady_clock::now(), e.position);
                }

            }
        });

    m_messenger->set_handler<PlayerJoinResponseMessage>(
        [&](PlayerJoinResponseMessage* mesg) {
            if(!m_is_connected) {
                std::println("Player joined!");
                m_entity_id = (entt::entity)mesg->entity_id;
                m_is_connected = true;
                m_entity_mapping[mesg->entity_id] = m_player_entity;

                auto* body = m_world->registry().try_get<Transform>(m_player_entity);
                body->set_position(mesg->position);
                // auto* body = m_world->registry().try_get<CharacterBody>(m_player_entity);
                // if(body) {
                //     body->m_character->SetPosition(JPH::Vec3(mesg->position.x, mesg->position.y, mesg->position.z));
                // }
            }
        });


    PlayerJoinRequestMessage join_request = {
        .name = "Test Player"
    };

    m_messenger->send(join_request);
}

void ClientWorldController::update(double delta_time) {
    m_player_controller.update(m_input_manager, delta_time);
    
    if(m_tick_step.update()) {
        m_messenger->update();

        if(!m_is_connected) {
            return;
        } else {
            CharacterController& character = m_world->registry().get<CharacterController>(m_player_entity);
            character.set_input(m_frame_idx, m_player_controller.input());

            PlayerInputMessage player_input_mesg = {
                .frame_idx = m_frame_idx,
                .entity_id = (uint32_t)m_entity_id,
                .inputs = m_player_controller.input()
            };

            m_messenger->send(player_input_mesg);

            m_physics_world->step(m_frame_idx, m_tick_step.delta_time());
            CharacterBody& ts = m_world->registry().get<CharacterBody>(m_player_entity);
            auto position = ts.m_character->GetPosition();
            character.position_history().set(m_frame_idx, glm::vec3(position[0], position[1], position[2]));
            EntityInterpolation& interpolation = m_world->registry().get<EntityInterpolation>(m_player_entity);
            interpolation.push(std::chrono::steady_clock::now(), glm::vec3(position[0], position[1], position[2]));

            m_frame_idx++;
        }
    }

    m_world->registry().view<EntityInterpolation, Transform>()
        .each([&](const auto entity, const EntityInterpolation& interpolation, Transform& ts) {
                if(entity == m_player_entity) {
                    auto now = std::chrono::steady_clock::now() - std::chrono::milliseconds(50);

                    auto [from, to, value] = interpolation.get_values_around(now);
                    ts.set_position(glm::mix(from, to, value));
                } else {
                    auto now = std::chrono::steady_clock::now() - std::chrono::milliseconds(300);

                    auto [from, to, value] = interpolation.get_values_around(now);
                    ts.set_position(glm::mix(from, to, value));
                }
        });

    // auto character = m_world->registry().try_get<CharacterBody>(m_player_entity);
    // character->set_input(m_player_controller.input());


}

}
