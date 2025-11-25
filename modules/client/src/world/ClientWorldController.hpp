#pragma once

#include <entt/entt.hpp>

#include "Address.hpp"
#include "HistoryBuffer.hpp"
#include "MessageHandler.hpp"
#include "entt/entity/fwd.hpp"
#include "io/InputState.hpp"
#include "runtime/LockStep.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/World.hpp"
#include "draw/WorldRenderer.hpp"
#include "world/CharacterBody.hpp"
#include "world/ThirdPersonPlayerController.hpp"

namespace tw {

/**
 * Multiplayer player controller. Requires address of a server to work.
 */
class ClientWorldController {
    const io::InputManager* m_input_manager;

    World* m_world;
    drw::WorldRenderer* m_world_renderer;
    JoltPhysicsWorld* m_physics_world;

    entt::entity m_player_entity;
    ThirdPersonPlayerController m_player_controller;
    
    std::optional<tw::net::MessageHandler> m_messenger;

    LockStep m_tick_step;

    uint32_t m_frame_idx = 1;
    entt::entity m_entity_id;

    glm::vec3 m_input;

    bool m_is_connected;

    std::optional<drw::Mesh> m_mesh;

    HistoryBuffer<uint32_t, glm::vec3, 40 * 10> m_position_history;

    /**
     * Mapping from the server entity_id to local entity_id
     * Server might have the same entity under different name
     * TODO: Figure out if entt supports custom IDs to sync them
     */
    std::unordered_map<uint32_t, entt::entity> m_entity_mapping;

    entt::entity create_entity(const std::string& name, glm::vec3 position);

public:
    GET(m_position_history, position_history);

    ClientWorldController(
            const io::InputManager* inputs,
            World* world,
            JoltPhysicsWorld* physics_world,
            drw::WorldRenderer* world_renderer,
            tw::net::Address address
    );

    void update(double delta_time);
};

}
