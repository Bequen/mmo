#pragma once

#include <cstdint>
#include <random>

namespace tw::net::quicr {

static uint64_t generate_id() {
    static std::mt19937_64 rng(std::random_device{}());
    return rng() & 0x3FFFFFFFFFFFFFFF;
}

}
