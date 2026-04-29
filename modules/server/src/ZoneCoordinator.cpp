#include "ZoneCoordinator.hpp"
#include "systems/Interest.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace tw::net {

ZoneCoordinator::ZoneCoordinator(std::string host, int port, std::string csv_path, im::AreaBounds bounds, uint32_t max_zones)
    : m_own_host(std::move(host)),
      m_own_port(port),
      m_csv_path(std::move(csv_path)),
      m_bounds(bounds),
      m_max_zones(max_zones)
{
}

// Recursively bisects `world` to find the cell for `index` out of `total` zones.
// Splits along the longest axis; left/bottom half gets the lower indices.
static im::AreaBounds compute_zone_bounds(uint32_t index, uint32_t total, im::AreaBounds world) {
    if (total <= 1) return world;

    uint32_t left_count = total / 2;
    float dx = world.max.x - world.min.x;
    float dz = world.max.y - world.min.y;

    im::AreaBounds left, right;
    if (dx >= dz) {
        float mid  = (world.min.x + world.max.x) * 0.5f;
        left  = { world.min,           { mid,         world.max.y } };
        right = { { mid, world.min.y }, world.max                  };
    } else {
        float mid  = (world.min.y + world.max.y) * 0.5f;
        left  = { world.min,           { world.max.x, mid         } };
        right = { { world.min.x, mid }, world.max                  };
    }

    if (index < left_count)
        return compute_zone_bounds(index,              left_count,           left);
    else
        return compute_zone_bounds(index - left_count, total - left_count,   right);
}

ZoneRegistration ZoneCoordinator::register_zone() {
    int fd = open(m_csv_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        spdlog::error("ZoneCoordinator: failed to open registry file '{}'", m_csv_path);
        return {};
    }

    flock(fd, LOCK_EX);

    // Read all existing entries.
    // CSV format: id,host,port,min_x,min_z,max_x,max_z
    std::vector<ZoneAddress> peers;
    {
        std::ifstream in(m_csv_path);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string id_s, host, port_s, min_x_s, min_z_s, max_x_s, max_z_s;
            if (!std::getline(ss, id_s,   ',')) continue;
            if (!std::getline(ss, host,   ',')) continue;
            if (!std::getline(ss, port_s, ',')) continue;
            if (!std::getline(ss, min_x_s,',')) continue;
            if (!std::getline(ss, min_z_s,',')) continue;
            if (!std::getline(ss, max_x_s,',')) continue;
            if (!std::getline(ss, max_z_s,',')) continue;
            peers.push_back({
                static_cast<uint32_t>(std::stoul(id_s)),
                std::move(host),
                std::stoi(port_s),
                { { std::stof(min_x_s), std::stof(min_z_s) },
                  { std::stof(max_x_s), std::stof(max_z_s) } }
            });
        }
    }

    uint32_t assigned_id = static_cast<uint32_t>(peers.size()) + 1;
    im::AreaBounds new_bounds = compute_zone_bounds(peers.size(), m_max_zones, m_bounds);

    {
        std::ofstream out(m_csv_path, std::ios::app);
        out << assigned_id      << ','
            << m_own_host       << ','
            << m_own_port       << ','
            << new_bounds.min.x << ',' << new_bounds.min.y << ','
            << new_bounds.max.x << ',' << new_bounds.max.y << '\n';
    }

    flock(fd, LOCK_UN);
    close(fd);

    return { assigned_id, new_bounds, std::move(peers) };
}

} // namespace tw::net
