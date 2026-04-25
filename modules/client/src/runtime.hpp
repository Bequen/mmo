#pragma once

#include "draw/WorldRenderer.hpp"
#include "io/InputState.hpp"
#include "runtime/LockStep.hpp"
#include "world/ClientWorldController.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/World.hpp"

#include "messages/PlayerMoveMessage.hpp"

namespace tw {

/**
 * Handles runtime of the client application.
 */
class Runtime {
private:
    io::Files m_files;

    std::unique_ptr<lft::win::Window> m_window;

    tw::World m_world;

    JoltPhysicsWorld m_physics_world;

    tw::drw::WorldRenderer m_world_renderer;

    tw::io::InputManager m_input_manager;

    tw::ClientWorldController m_world_controller;

    tw::LockStep m_lockstep;

    bool m_is_running;

    std::unordered_map<uint32_t, entt::entity> m_players;

    void send_player_positions();

public:
    const bool is_running() const {
        return m_is_running;
    }

    Runtime(int argc, char** argv);

    void run();
};

}
