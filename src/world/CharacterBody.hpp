#pragma once

#include "metrics/HistoryBuffer.hpp"
#include <glm/glm.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace tw {

class CharacterBody {
public:
    JPH::CharacterVirtual* m_character;

    JPH::Vec3 m_desired_velocity;

    CharacterBody(JPH::CharacterVirtual* character) :
        m_character(character)
    { }
};

}
