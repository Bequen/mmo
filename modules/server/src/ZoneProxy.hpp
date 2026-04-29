#pragma once

#include "systems/Interest.hpp"

#include <glm/glm.hpp>
#include <string>

namespace tw::net {

// Pure game-state snapshot used to transfer an entity between zones.
// Session/client mapping is the owner's (e.g. ZoneServer's) responsibility.
struct EntityInfo {
    std::string name;
    glm::vec3   position;
};

// Abstract handle to a zone from the outside.
// Implementations may be local (same process) or remote (over the network).
class ZoneProxy {
public:
    virtual ~ZoneProxy() = default;

    // The geographic area this zone owns in the XZ plane.
    virtual im::AreaBounds area() const = 0;

    // Accept an entity transferred from another zone.
    // The callee spawns the entity in its own world.
    // Session routing (calling add_client on the receiving zone) is the
    // owner's responsibility and must happen separately.
    virtual void transfer_entity(EntityInfo&& info) = 0;
};

} // namespace tw::net
