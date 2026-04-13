#pragma once

/**
 * tw::serial — umbrella include
 *
 * Include this single header to get the full serialisation API:
 *   - BinaryBuffer  (backing store)
 *   - BinaryWriter  (write cursor + Codec dispatch)
 *   - BinaryReader  (read cursor + Codec dispatch)
 *   - Codec<T>      (extensible type trait)
 *   - Built-in Codec specialisations for all C++ primitive types
 *   - GlmCodec      (vec2, vec3, vec4, mat4)
 *   - EnttCodec     (entt::entity as uint32)
 *   - WorldStateWriter / WorldStateReader  (game-specific high-level API)
 */

#include "BinaryBuffer.hpp"
#include "Codec.hpp"
#include "GlmCodec.hpp"
#include "EnttCodec.hpp"
#include "WorldStateWriter.hpp"
