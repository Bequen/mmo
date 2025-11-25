#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace tw {

#define CHUNK_WIDTH 16

struct ChunkData {
    using Type = uint8_t;

    std::vector<Type> data;
    
    ChunkData() :
        data(CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH)
    {
    }

    Type get(glm::ivec3 position) {
        assert(position.x >= 0 && position.x < CHUNK_WIDTH);
        assert(position.y >= 0 && position.y < CHUNK_WIDTH);
        assert(position.z >= 0 && position.z < CHUNK_WIDTH);

        return data[position.x + position.y * CHUNK_WIDTH + position.z * CHUNK_WIDTH * CHUNK_WIDTH];
    }
};

}
