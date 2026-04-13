#pragma once

/**
 * Codec specialisations for GLM types.
 *
 * Include this header in any translation unit that needs to
 * encode/decode GLM vectors or matrices.
 *
 * Wire format (little-endian, matches the host layout on x86/ARM LE):
 *   vec2  — 2 × float  (8 bytes)
 *   vec3  — 3 × float  (12 bytes)
 *   vec4  — 4 × float  (16 bytes)
 *   mat4  — 16 × float (64 bytes, column-major, matching GLM's default)
 */

#include "Codec.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace tw::serial {

template<>
struct Codec<glm::vec2> {
    static void encode(BinaryWriter& w, const glm::vec2& v) noexcept {
        w.write(v.x);
        w.write(v.y);
    }
    static glm::vec2 decode(BinaryReader& r) noexcept {
        glm::vec2 v;
        v.x = r.read<float>();
        v.y = r.read<float>();
        return v;
    }
};

template<>
struct Codec<glm::vec3> {
    static void encode(BinaryWriter& w, const glm::vec3& v) noexcept {
        // vec3 is 3 contiguous floats in GLM's layout
        w.write_bytes(&v.x, 3 * sizeof(float));
    }
    static glm::vec3 decode(BinaryReader& r) noexcept {
        glm::vec3 v;
        r.read_bytes(&v.x, 3 * sizeof(float));
        return v;
    }
};

template<>
struct Codec<glm::vec4> {
    static void encode(BinaryWriter& w, const glm::vec4& v) noexcept {
        w.write_bytes(&v.x, 4 * sizeof(float));
    }
    static glm::vec4 decode(BinaryReader& r) noexcept {
        glm::vec4 v;
        r.read_bytes(&v.x, 4 * sizeof(float));
        return v;
    }
};

template<>
struct Codec<glm::mat4> {
    static void encode(BinaryWriter& w, const glm::mat4& m) noexcept {
        // GLM mat4 is column-major; 16 contiguous floats
        w.write_bytes(&m[0][0], 16 * sizeof(float));
    }
    static glm::mat4 decode(BinaryReader& r) noexcept {
        glm::mat4 m;
        r.read_bytes(&m[0][0], 16 * sizeof(float));
        return m;
    }
};

} // namespace tw::serial
