#include "ClientWorldController.hpp"

#include <glm/glm.hpp>
#include <chrono>
#include <spdlog/spdlog.h>

#include "Address.hpp"

#include "Entity.pb.h"
#include "Login.pb.h"
#include "WorldState.pb.h"
#include "PlayerMove.pb.h"

#include "entt/entity/entity.hpp"
#include "messenger/MessageHandler.hpp"
#include "messenger/Messenger.hpp"
#include "TcpStream.hpp"
#include "messages/PlayerMoveMessage.hpp"
#include "metrics/HistoryBuffer.hpp"
#include "world/CharacterBody.hpp"
#include "world/CharacterController.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/WorldEntity.hpp"

namespace tw {

typedef  HistoryBuffer<long, glm::vec3> EntityPositionHistory;

entt::entity create_player_entity(World* world, JoltPhysicsWorld* physics_world, drw::WorldRenderer* renderer) {
    // entt::entity entity = world->registry().create();

    // drw::Mesh mesh = renderer->add_mesh(drw::MeshData::cube(glm::vec3(1.0f)));

    // world->registry()
    //     .emplace<Transform>(entity,
    //             Transform(glm::vec3(0.0f, 10.0f, 0.0f)));

    // world->registry()
    //     .emplace<EntityPositionHistory>(entity, 0, glm::vec3(0.0f, 10.0f, 0.0f), 1000);

    // world->registry()
    //     .emplace<tw::WorldEntity>(entity,
    //             tw::WorldEntity(std::string("player"), (uint32_t)entity));

    // world->registry()
    //     .emplace<tw::drw::Mesh>(entity, mesh);

    // world->registry()
    //     .emplace<tw::CharacterController>(entity, 20.0f);
    // auto* body = &world->registry()
    //     .emplace<CharacterBody>(entity, physics_world->create_character(
    //             new JPH::BoxShape(JPH::Vec3Arg(0.5f, 0.5f, 0.5f)),
    //             glm::vec3(0.0f, 10.0f, 0.0f)));


    // return entity;
    return entt::entity(0);
}


entt::entity
ClientWorldController::create_entity(const std::string& name, glm::vec3 position) {
    const auto entity = m_world->registry().create();

    if(!m_mesh.has_value()) {
        m_mesh = m_world_renderer->add_mesh(drw::MeshData::cube(glm::vec3(1.0f)));
    }
    spdlog::info("Creating entity {}", name);

    m_world->registry()
        .emplace<Transform>(entity,
                Transform(position));

    m_world->registry()
        .emplace<tw::WorldEntity>(entity,
                tw::WorldEntity(name, (uint32_t)entity));

    m_world->registry()
        .emplace<tw::drw::Mesh>(entity,
                m_mesh.value());

    return entity;
}

net::TcpStream create_stream(tw::net::Address address) {
    auto stream = net::TcpStream::connect(address);
    if(!stream.has_value()) {
        spdlog::error("Failed to connect to server");
        throw std::runtime_error("Failed to connect to server");
    }

    stream.value().set_non_blocking();

    return std::move(stream.value());
}

std::optional<entt::entity> ClientWorldController::map_from_server_entity(int id) {
    if(m_entity_mapping.contains(id)) {
        return m_entity_mapping[id];
    }

    return {};
}

void ClientWorldController::map_server_entity(int server_id, entt::entity local_id) {
    m_entity_mapping[server_id] = local_id;
}

void ClientWorldController::apply_entity_positions(const mmo::EntityPosition* const* positions, size_t count) {
    for(int i = 0; i < count; i++) {
        const mmo::EntityPosition* const position = positions[i];
        std::optional<entt::entity> entity = map_from_server_entity(position->id());

        if(!entity.has_value()) {
            spdlog::warn("Received position update for unknown entity {}", position->id());
            continue;
        }

        glm::vec3 p = {position->x(), position->y(), position->z()};
        m_entity_interpolator.add_position_for_entity(entity.value(), p);

        EntityPositionHistory* history = m_world->registry().try_get<EntityPositionHistory>(entity.value());

        if(history != nullptr) {
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
            history->set(millis, p);
        }
    }
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
    m_messenger{address},
    m_tick_step(20),
    m_is_connected(false),
    m_position_history_exporter("/home/martin/output.csv"),
    m_entity_interpolator(&m_world->registry(), m_player_entity, 300)
{
    m_messenger->set_handler<mmo::LoginResponse>(
        [&](mmo::LoginResponse* mesg) {
            if(!m_is_connected) {
                spdlog::info("Joined the game!");
            }
        });

    m_messenger->set_handler<mmo::WorldStateMessage>(
        [&](mmo::WorldStateMessage* mesg) {
            apply_entity_positions(mesg->entities().data(), mesg->entities_size());

            for(const auto& spawn : mesg->spawns()) {
                auto entity = create_entity("test", glm::vec3());

                map_server_entity(spawn.id(), entity);

                m_entity_interpolator.register_entity(entity);
            }
        });

    m_messenger->set_handler<mmo::EntitySpawnMessage>(
        [&](mmo::EntitySpawnMessage* mesg) {
            auto entity = create_entity(mesg->name(), glm::vec3());

            map_server_entity(mesg->entity_id(), entity);

            m_entity_interpolator.register_entity(entity);
        });

    PlayerJoinRequestMessage join_request = {
        .name = "Test Player"
    };

    mmo::LoginRequest login_request = { };
    login_request.set_username("martin");
    login_request.set_password("martin");

    auto r = m_messenger->send(login_request);
}

ClientWorldController::~ClientWorldController() {
}

void ClientWorldController::export_entity_history() {
}

void ClientWorldController::update(double delta_time) {
    m_player_controller.update(m_input_manager, delta_time);

    ImGui::Begin("Player Controller");

    ImGui::Text("Player entity ID: %d", (uint32_t)m_player_entity);
    for(auto mapping : m_entity_mapping) {
        ImGui::Text("%d -> %d", (uint32_t)mapping.first, mapping.second);
    }

    ImGui::End();

    if(m_tick_step.update()) {
        m_messenger->update();

        if(!m_is_connected && false) {
            return;
        } else {

            // CharacterController& character = m_world->registry().get<CharacterController>(m_player_entity);
            // character.set_input(m_frame_idx, m_player_controller.input());

            mmo::PlayerMoveMessage player_move_message = {};

            player_move_message.set_frame_idx(m_frame_idx);
            mmo::PlayerInput* player_input = new mmo::PlayerInput();
            player_input->set_x(m_player_controller.input().x);
            player_input->set_y(m_player_controller.input().y);
            player_input->set_z(m_player_controller.input().z);

            player_move_message.set_allocated_input(player_input);
            auto r = m_messenger->send(player_move_message);

            // CharacterBody& ts = m_world->registry().get<CharacterBody>(m_player_entity);
            // auto position = ts.m_character->GetPosition();
            // character.position_history().set(m_frame_idx, glm::vec3(position[0], position[1], position[2]));
            // EntityInterpolation& interpolation = m_world->registry().get<EntityInterpolation>(m_player_entity);
            // interpolation.push(std::chrono::steady_clock::now(), glm::vec3(position[0], position[1], position[2]));

            // m_player_controller.set_target(glm::vec3(position[0], position[1], position[2]));
            //
            export_entity_history();

            m_frame_idx++;

            m_world->registry().view<Transform>()
                .each([&](const auto e, Transform& t) {
                    m_position_history_exporter.write((uint32_t)e,
                        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count(), t.position());
                });
        }
    }

    m_physics_world->step(m_frame_idx, delta_time);

    m_entity_interpolator.update();
}

}
