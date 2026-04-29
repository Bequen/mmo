#include "JoltPhysicsWorld.hpp"

#include <cmath>
#include <iostream>
#include <print>
#include <cstring>
#include <tracy/Tracy.hpp>

#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Physics/EActivation.h"
#include "Jolt/Physics/StateRecorderImpl.h"
#include "Jolt/RegisterTypes.h"

#include "World.hpp"
#include "world/CharacterBody.hpp"
#include "world/CharacterController.hpp"
#include "world/RigidBody.hpp"
#include "world/Transform.hpp"

namespace tw {

bool JoltPhysicsWorld::initialization() {
    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();

    JPH::RegisterTypes();

    return true;
}


JoltPhysicsWorld::JoltPhysicsWorld(World* world) :
    m_is_initialized(initialization()),
    temp_allocator(std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024)),
    job_system(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1),
    m_world(world),
    m_history(0, std::move(std::make_unique<JPH::StateRecorderImpl>()), 10),
    m_latest_frame(0),
    broad_phase_layer_interface(),
    object_vs_broadphase_layer_filter(),
    object_vs_object_layer_filter(),
    physics_system(),
    body_activation_listener(),
    contact_listener(),
    m_character_vs_character_collision()
{
    // for(int i = 0; i < m_thread_pool.size(); i++) {
    //     temp_allocator.emplace_back(std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024));
    // }
    // for(auto& allocator : temp_allocator) {
    //     // allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    // }

    const uint32_t MAX_BODIES = 1024;
    const uint32_t BODY_MUTEXES = 0;
    const uint32_t MAX_BODY_PAIRS = 1024;
    const uint32_t MAX_CONTACT_CONSTRAINTS = 1024;

	physics_system.Init(MAX_BODIES, BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS,
            broad_phase_layer_interface,
            object_vs_broadphase_layer_filter,
            object_vs_object_layer_filter);

	physics_system.SetBodyActivationListener(&body_activation_listener);

	physics_system.SetContactListener(&contact_listener);

    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
	floor_shape_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();

    JPH::BodyCreationSettings floor_settings(
            floor_shape, JPH::RVec3(0.0, 0., 0.0),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING);

    JPH::Body *floor = body_interface().CreateBody(floor_settings);
    body_interface().AddBody(floor->GetID(), JPH::EActivation::DontActivate);


	// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
	// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
	// body_interface().SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -5.0f, 0.0f));
}

void JoltPhysicsWorld::update(uint32_t frame_idx, double delta_time) {
    auto view = m_world->registry().view<CharacterController, CharacterBody>();

    view.each([&](entt::entity entity, const CharacterController& controller, CharacterBody& rb) {
        auto allocator = temp_allocator.get();

        ZoneScoped;
        ZoneNameF("CharacterController update %d", (uint32_t)0);
        auto character = rb.m_character;

        bool player_controls_horizontal_velocity = true || character->IsSupported();

        bool sEnableCharacterInertia = true;
        bool mAllowSliding = true;

        if (player_controls_horizontal_velocity) {
            glm::vec3 input = controller.input(frame_idx);
            // Smooth the player input
            JPH::Vec3 vel = JPH::Vec3(input.x, input.y, input.z) * controller.speed();
            float inertia_factor = 1.0f - std::exp(-delta_time * 10.0f); // 10.0 = responsiveness tuning knob
            rb.m_desired_velocity = sEnableCharacterInertia
                ? rb.m_desired_velocity + inertia_factor * (vel - rb.m_desired_velocity)
                : vel;

            // True if the player intended to move
            mAllowSliding = input.length() < 0.01f; // allow sliding when idle, prevent when moving
        }
        JPH::Vec3 current_vertical_velocity = character->GetLinearVelocity().Dot(character->GetUp()) * character->GetUp();
        JPH::Vec3 ground_velocity = character->GetGroundVelocity();
        JPH::Vec3 new_velocity;

        bool moving_towards_ground = (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f;
        if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround	// If on ground
            && (sEnableCharacterInertia ?
                moving_towards_ground													// Inertia enabled: And not moving away from ground
                : !character->IsSlopeTooSteep(character->GetGroundNormal())))			// Inertia disabled: And not on a slope that is too steep
        {
            // Assume velocity of ground when on ground
            new_velocity = ground_velocity;

            // Jump
            // if (inJump && moving_towards_ground)
            //     new_velocity += sJumpSpeed * mCharacter->GetUp();
        }
        else {
            new_velocity = current_vertical_velocity;
        }

        auto character_up_rotation = character->GetUp();
        // Gravity
        new_velocity += (physics_system.GetGravity()) * delta_time;
        new_velocity += rb.m_desired_velocity;

        if (player_controls_horizontal_velocity)
        {
            // Player input
        }
        else
        {
            // Preserve horizontal velocity
            JPH::Vec3 current_horizontal_velocity = character->GetLinearVelocity() - current_vertical_velocity;
            new_velocity += current_horizontal_velocity;
        }

        // Update character velocity
        character->SetLinearVelocity(new_velocity);

        // auto velocity = character->GetLinearVelocity() + physics_system.GetGravity() * delta_time ;
        // character->SetLinearVelocity(velocity);
        JPH::CharacterVirtual::ExtendedUpdateSettings update_settings = {};
        update_settings.mStickToFloorStepDown = mAllowSliding
            ? JPH::Vec3(0, -0.5f, 0)   // snap to floor when idle
            : JPH::Vec3::sZero();
        update_settings.mWalkStairsStepUp = JPH::Vec3(0, 0.4f, 0); // re-enable stair stepping

        {
            ZoneScopedN("Extended update");
            character->ExtendedUpdate(delta_time,
                -character->GetUp() * physics_system.GetGravity().Length(),
                update_settings,
                physics_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                physics_system.GetDefaultLayerFilter(Layers::MOVING),
                {},
                {},
                *allocator);
        }
    });

    physics_system.Update(delta_time, 1, temp_allocator.get(), &job_system);

    auto stateRecorder = std::make_shared<JPH::StateRecorderImpl>();
    physics_system.SaveState(*stateRecorder.get());

    m_history.set(frame_idx, stateRecorder);
}

void JoltPhysicsWorld::step(uint32_t frame_idx, double delta_time) {
    update(frame_idx, delta_time);

    m_world->registry().view<Transform, CharacterBody>()
        .each([&](Transform& ts, CharacterBody& rb) {
                auto character = rb.m_character;

                auto transform = character->GetWorldTransform();
                memcpy(&ts.transform, &transform, sizeof(glm::mat4));
        });

    // copy values
    // m_world->registry().view<Transform, RigidBody>()
    //     .each([&](Transform& ts, RigidBody& rb) {
    //             auto transform = body_interface().GetWorldTransform(rb.id);
    //             memcpy(&ts.transform, &transform, sizeof(glm::mat4));
    //     });
    //
    // m_world->registry().view<Transform, CharacterBody>()
    //     .each([&](Transform& ts, CharacterBody& rb) {
    //             auto character = rb.m_character;
    //
    //             auto transform = character->GetWorldTransform();
    //             memcpy(&ts.transform, &transform, sizeof(glm::mat4));
    //     });

    m_latest_frame = std::max(m_latest_frame, frame_idx);
}

}
