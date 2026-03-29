#pragma once

#include <entt/entt.hpp>
#include <glm/gtx/io.hpp>

#include "Address.hpp"
#include "TcpStream.hpp"
#include "messenger/MessageHandler.hpp"
#include "entt/entity/fwd.hpp"
#include "io/InputState.hpp"
#include "metrics/HistoryBufferExporter.hpp"
#include "runtime/LockStep.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/World.hpp"
#include "draw/WorldRenderer.hpp"
#include "world/ThirdPersonPlayerController.hpp"
#include "network/EntityPositionInterpolator.hpp"


namespace tw {

/**
 * Multiplayer player controller. Requires address of a server to work.
 */
class ClientWorldController {
    /**
     * Inputs
     */
    const io::InputManager* m_input_manager;

    /**
     * Pointers to the world
     */
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

    using Clock = std::chrono::steady_clock;
    HistoryBufferExporter<long, glm::vec3> m_position_history_exporter;

    net::EntityPositionInterpolator m_entity_interpolator;

    /**
     * Mapping from the server entity_id to local entity_id
     * Server might have the same entity under different name
     * TODO: Figure out if entt supports custUntitledom IDs to sync them
     */
    std::unordered_map<uint32_t, entt::entity> m_entity_mapping;

    entt::entity create_entity(const std::string& name, glm::vec3 position);

    net::MessageHandler create_messenger();

    std::optional<entt::entity> map_from_server_entity(int id);

    void map_server_entity(int server_id, entt::entity local_id);

    void apply_entity_positions(const mmo::EntityPosition* const* positions, size_t count);



    /**
     * Exports entity history to CSV file for analysis
     */
    void export_entity_history();

public:
    ClientWorldController(
            const io::InputManager* inputs,
            World* world,
            JoltPhysicsWorld* physics_world,
            drw::WorldRenderer* world_renderer,
            tw::net::Address address
    );

    ~ClientWorldController();

    void update(double delta_time);
};

}
