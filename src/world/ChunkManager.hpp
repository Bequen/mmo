#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <optional>

#include "ChunkData.hpp"

namespace tw {

class ChunkManager {
public:
    ChunkManager(uint32_t render_distance);

    std::optional<ChunkData&> get_chunk(glm::ivec2 position);
};

}
