#pragma once

#include "FastNoise/Generators/Fractal.h"
#include "FastNoise/SmartNode.h"
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <FastNoise/FastNoise.h>

namespace tw {

class Chunk {
public:
    std::vector<uint8_t> m_voxels;
};

class TerrainGenerator {
    uint32_t m_seed;

    FastNoise::SmartNode<FastNoise::Simplex> m_fn_simplex;
    FastNoise::SmartNode<FastNoise::FractalFBm> m_fn_fractal;

public:
    TerrainGenerator(uint32_t seed) {
        m_seed = seed;

        m_fn_simplex = FastNoise::New<FastNoise::Simplex>();
        m_fn_fractal = FastNoise::New<FastNoise::FractalFBm>();

        m_fn_fractal->SetSource(m_fn_simplex);
        m_fn_fractal->SetOctaveCount(5);
    }

    Chunk generate(glm::ivec3 offset) {
        std::vector<float> noiseOutput(16 * 16);
        m_fn_simplex->GenUniformGrid2D(noiseOutput.data(), 0, 0, 16, 16, 0.2f, m_seed);

        std::vector<uint8_t> chunk(16 * 16 * 16);

        for(uint32_t y = 0; y < 16; y++) {
            for(uint32_t x = 0; x < 16; x++) {
                for(uint32_t z = 0; z < noiseOutput[x + y * 16] * 8.0f; z++) {
                    chunk[x + y * 16 + z * 16 * 16] = 1;
                }
            }
        }

        return Chunk {chunk};
    }
};

}
