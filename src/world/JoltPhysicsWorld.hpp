#pragma once

#include "world/RigidBody.hpp"
#include <iostream>
#include <print>
#include "metrics/HistoryBuffer.hpp"

#include <glm/glm.hpp>
#include <Jolt/Jolt.h>

#include <Jolt/Physics/StateRecorderImpl.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace tw {

namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return true; // return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return inObject2 == Layers::NON_MOVING; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char * GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
	{
		switch ((JPH::BroadPhaseLayer::Type)inLayer)
		{
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
            default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
    		return true;
			// return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return inLayer2 == BroadPhaseLayers::NON_MOVING;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

class MyContactListener : public JPH::ContactListener
{
public:
	// See: ContactListener
	virtual JPH::ValidateResult	OnContactValidate(
            const JPH::Body &inBody1,
            const JPH::Body &inBody2,
            JPH::RVec3Arg inBaseOffset,
            const JPH::CollideShapeResult &inCollisionResult
    ) override {
        std::cout << "Contact validate callback" << std::endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(
            const JPH::Body &inBody1,
            const JPH::Body &inBody2,
            const JPH::ContactManifold &inManifold,
            JPH::ContactSettings &ioSettings
    ) override
	{
        std::cout << "A contact was added" << std::endl;
	}

	virtual void OnContactPersisted(
            const JPH::Body &inBody1,
            const JPH::Body &inBody2,
            const JPH::ContactManifold &inManifold,
            JPH::ContactSettings &ioSettings
    ) override
	{
        std::cout << "A contact was persisted" << std::endl;
	}

	virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
	{
        std::cout << "A contact was removed" << std::endl;
	}
};

class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
	virtual void OnBodyActivated(const JPH::BodyID &inBodyID, uint64_t inBodyUserData) override
	{
        std::cout << "A body got activated" << std::endl;
	}

	virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, uint64_t inBodyUserData) override
	{
        std::cout << "A body went to sleep" << std::endl;
	}
};

class World;

class JoltPhysicsWorld {
private:
    std::vector<std::unique_ptr<JPH::TempAllocatorImpl>> temp_allocator;
    JPH::JobSystemThreadPool job_system;

    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    JPH::PhysicsSystem physics_system;
    MyBodyActivationListener body_activation_listener;
    MyContactListener contact_listener;

    JPH::CharacterVsCharacterCollisionSimple m_character_vs_character_collision;

    HistoryBuffer<int, std::shared_ptr<JPH::StateRecorderImpl>> m_history;

    inline JPH::BodyInterface &body_interface() {
        return physics_system.GetBodyInterface();
    }

    JPH::BodyID sphere_id;

    World* m_world;

    uint32_t m_latest_frame;

    void update(uint32_t frame_idx, double delta_time);

public:
    JoltPhysicsWorld(World* world);

    RigidBody create_dynamic_rigid_body(JPH::Shape* shape, glm::vec3 position) {
        JPH::BodyCreationSettings settings(
                shape,
                JPH::RVec3(position.x, position.y, position.z),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic,
                Layers::MOVING);

        auto id = body_interface().CreateAndAddBody(settings, JPH::EActivation::Activate);

        return {id};
    }

    JPH::CharacterVirtual* create_character(JPH::Shape* shape, glm::vec3 position) {
        JPH::CharacterVirtualSettings settings;
        settings.mShape = shape;
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.5f);

        auto* character = new JPH::CharacterVirtual(&settings,
                JPH::RVec3(position.x, position.y, position.z),
                JPH::Quat::sIdentity(),
                0, &physics_system);
        character->SetCharacterVsCharacterCollision(&m_character_vs_character_collision);
        // m_character_vs_character_collision.Add(character);

        return character;
    }

    void move_character(JPH::CharacterVirtual* character, glm::vec3 force) {
        character->SetLinearVelocity(JPH::Vec3(force.x, force.y, force.z));
    }

    void add_force(RigidBody& rigidbody, glm::vec3 force) {
        body_interface().AddForce(rigidbody.id, JPH::RVec3(force.x, force.y, force.z));
    }

    void step(uint32_t frame_idx, double delta_time);

    void rollback(uint32_t frame) {
        if(frame == 0) return;
        auto snapshot = m_history.get(frame);

        if(!snapshot.has_value()) {
            throw std::runtime_error("Failed to restore history");
        }

        auto snapshot_value = snapshot.value()->get();
        physics_system.RestoreState(*snapshot_value);
    }

    void apply_rollback(uint32_t frame) {
        for(uint32_t i = frame; i < m_latest_frame; i++) {
            update(i, 1000.0f / 20.0f);
        }
    }
};

}
