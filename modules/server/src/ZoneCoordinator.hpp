#pragma once

#include "systems/Interest.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tw::net {

struct ZoneAddress {
    uint32_t       id;
    std::string    host;
    int            port;
    im::AreaBounds bounds;
};

struct ZoneRegistration {
    uint32_t             assigned_id;
    im::AreaBounds       area_bounds;
    std::vector<ZoneAddress> peers;
};

// Service-discovery stub: each zone server registers its zones here and
// receives back the addresses of all previously registered zones so it can
// establish peer connections.
//
// First implementation persists registrations to a shared CSV file
// (id,host,port,min_x,min_z,max_x,max_z).  Concurrent access is serialised
// with an advisory flock so multiple processes can share the same file safely.
//
// Intended to be replaced by an etcd-backed implementation later.
class ZoneCoordinator {
    std::string    m_own_host;
    int            m_own_port;
    std::string    m_csv_path;
    im::AreaBounds m_bounds;
    uint32_t       m_max_zones;

public:
    ZoneCoordinator(
        std::string    host,
        int            port,
        std::string    csv_path  = "/tmp/tw_zone_registry.csv",
        im::AreaBounds bounds    = {{ -5000.f, -5000.f }, { 5000.f, 5000.f }},
        uint32_t       max_zones = 2
    );

    // Assigns this zone a unique, non-overlapping slice of the world (determined
    // by its registration index and max_zones), appends it to the registry, and
    // returns the authoritative bounds together with the addresses of all
    // previously registered peers.
    ZoneRegistration register_zone();
};

} // namespace tw::net
