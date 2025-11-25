#include "LocalWorldController.hpp"
#include "world/JoltPhysicsWorld.hpp"
#include "world/WorldEntity.hpp"

namespace tw {

CharacterBody* create_player_body(World* world, JoltPhysicsWorld* physics_world, drw::WorldRenderer* renderer) {
    const auto entity = world->registry().create();

    drw::Mesh mesh = renderer->add_mesh(drw::MeshData::cube(glm::vec3(1.0f)));

    world->registry()
        .emplace<Transform>(entity, 
                Transform(glm::vec3(0.0f, 10.0f, 0.0f)));

    world->registry()
        .emplace<tw::WorldEntity>(entity, 
                tw::WorldEntity(std::string("player"), (uint32_t)entity));

    world->registry()
        .emplace<tw::drw::Mesh>(entity, 
                mesh);
    
    auto* body = &world->registry()
        .emplace<CharacterBody>(entity, physics_world->create_character(
                new JPH::BoxShape(JPH::Vec3Arg(0.5f, 0.5f, 0.5f)), 
                glm::vec3(0.0f, 10.0f, 0.0f)));

    return body;
}

LocalWorldController::LocalWorldController(
        io::InputManager* inputs,
        World* world,
        JoltPhysicsWorld* physics_world,
        drw::WorldRenderer* world_renderer) :
    m_input_manager(inputs),
    m_world(world),
    m_physics_world(physics_world),
    m_world_renderer(world_renderer),
    m_player_body(create_player_body(world, physics_world, world_renderer)),
    m_player_controller(&world_renderer->camera(), glm::vec3())
{

}

void LocalWorldController::update(double delta_time) {
    m_player_controller.update(m_input_manager, delta_time);
}

}
