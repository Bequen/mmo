#pragma once

#include "draw/WorldRenderer.hpp"
#include "io/InputState.hpp"
#include "world/CharacterBody.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/ThirdPersonPlayerController.hpp"
#include "world/World.hpp"

namespace tw {

class LocalWorldController {
private:
    io::InputManager* m_input_manager;

    World* m_world;
    JoltPhysicsWorld* m_physics_world;
    drw::WorldRenderer* m_world_renderer;

    CharacterBody* m_player_body;
    ThirdPersonPlayerController m_player_controller;

public:
    LocalWorldController(
            io::InputManager* inputs,
            World* world,
            JoltPhysicsWorld* physics_world,
            drw::WorldRenderer* world_renderer);

    void update(double delta_time);
};

}
