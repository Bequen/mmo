#pragma once

#include <glm/glm.hpp>

#include "InterpolatedProperty.hpp"


namespace tw::net {

typedef InterpolatedProperty<glm::vec3, 60> EntityPositionInterpolation;

}
