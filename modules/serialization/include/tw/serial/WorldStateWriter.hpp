#pragma once

/**
 * WorldStateWriter / WorldStateReader
 * =====================================
 * Project-specific serialisation for the per-frame world-state snapshot
 * sent from the server to each connected client.
 *
 * Wire format (all values little-endian):
 *
 *   ┌──────────────────────────────────────────────────────────┐
 *   │ Header (12 bytes)                                        │
 *   │   packet_type  : uint32   (PacketType::WORLD_STATE = 3)  │
 *   │   frame_idx    : uint32                                  │
 *   │   entity_count : uint32   (number of position records)   │
 *   ├──────────────────────────────────────────────────────────┤
 *   │ Spawns section                                           │
 *   │   spawn_count  : uint32                                  │
 *   │   spawn[i].id  : uint32   × spawn_count                  │
 *   ├──────────────────────────────────────────────────────────┤
 *   │ Despawns section                                         │
 *   │   despawn_count : uint32                                 │
 *   │   despawn[i].id : uint32  × despawn_count                │
 *   ├──────────────────────────────────────────────────────────┤
 *   │ Entity positions (hot path — tightly packed)             │
 *   │   [ id:uint32, x:float, y:float, z:float ] × entity_count│
 *   └──────────────────────────────────────────────────────────┘
 *
 * Total minimum size : 20 bytes (header + empty spawns + empty despawns)
 * Per entity         : 16 bytes
 * 300 entities       : 20 + 300×16 = 4820 bytes  (well under MTU for segmented)
 *
 * Usage (server side, called once per client per frame):
 *
 *   tw::serial::WorldStateWriter w(buffer);      // buffer is a BinaryBuffer
 *   w.begin(frame_idx);
 *   w.write_spawns(interest.spawns());
 *   w.write_despawns(interest.despawns());
 *   w.begin_entities(num_entities);              // writes entity_count slot
 *   for each entity in interest.entities():
 *       w.write_entity(entity_id, position);
 *   w.end();                                     // patches entity_count
 *   // buffer.view() is ready to send
 *
 * Usage (client side):
 *
 *   tw::serial::WorldStateReader r(payload_span);
 *   auto header = r.read_header();               // frame_idx + counts
 *   for (auto id : r.read_spawns())   { ... }
 *   for (auto id : r.read_despawns()) { ... }
 *   while (r.has_entity()) {
 *       auto [id, pos] = r.read_entity();
 *       ...
 *   }
 */

#include "Codec.hpp"
#include "GlmCodec.hpp"
#include "EnttCodec.hpp"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <cstdint>
#include <span>
#include <spdlog/spdlog.h>

namespace tw::serial {

// ── Packet type tag ───────────────────────────────────────────────────────
// Mirrors PacketType::WORLD_STATE_PACKET (value 3) in packets/Packet.hpp.
// Hardcoded here so the serialisation module does not depend on the network
// module — the numerical value must stay in sync if the enum changes.
inline constexpr uint32_t kWorldStatePacketType = 3; // WORLD_STATE_PACKET

// ──────────────────────────────────────────────────────────────────────────
// WorldStateWriter
// ──────────────────────────────────────────────────────────────────────────

class WorldStateWriter {
    BinaryWriter  m_w;

    // Offsets for length-prefix patching
    std::size_t m_entity_count_offset{ 0 };
    uint32_t    m_entity_count{ 0 };

public:
    explicit WorldStateWriter(BinaryBuffer& buf) noexcept : m_w(buf) {}

    /**
     * Write the packet header.  Call once per frame, before everything else.
     * entity_count is patched in end().
     */
    void begin(uint32_t frame_idx) noexcept {
        m_entity_count = 0;

        // packet_type — lets the receiver dispatch without peeking further
        m_w.encode<uint32_t>(kWorldStatePacketType);

        // frame_idx
        m_w.encode<uint32_t>(frame_idx);

        // entity_count placeholder — patched when end() is called
        m_entity_count_offset = m_w.reserve_u32();
    }

    // ── Spawns ────────────────────────────────────────────────────────────

    /**
     * Write the spawn list.  Pass any range of entt::entity.
     */
    template<typename Range>
    void write_spawns(const Range& spawns) noexcept {
        auto count = static_cast<uint32_t>(std::size(spawns));
        m_w.encode<uint32_t>(count);
        for (const entt::entity e : spawns) {
            m_w.encode<entt::entity>(e);
        }
    }

    // ── Despawns ──────────────────────────────────────────────────────────

    template<typename Range>
    void write_despawns(const Range& despawns) noexcept {
        auto count = static_cast<uint32_t>(std::size(despawns));
        // m_w.encode<uint32_t>(count);
        for (const entt::entity e : despawns) {
            m_w.encode<entt::entity>(e);
        }
    }

    // ── Entity positions (the hot path) ───────────────────────────────────

    /**
     * Write a single entity position record.
     * id  : raw uint32 of the entity handle
     * pos : world-space position (x, y, z)
     *
     * This is the innermost loop of the replicator — every byte matters.
     * The compiler will inline both calls down to two contiguous memcpys.
     */
    void write_entity(uint32_t id, const glm::vec3& pos) noexcept {
        m_w.write(id);
        // Write x, y, z as 3 contiguous floats
        m_w.write_bytes(&pos.x, 3 * sizeof(float));
        ++m_entity_count;
    }

    // Convenience overload accepting an entt::entity handle directly
    void write_entity(entt::entity entity, const glm::vec3& pos) noexcept {
        write_entity(static_cast<uint32_t>(entity), pos);
    }

    /**
     * Patch the entity_count field written in begin() and finalise the
     * buffer.  Must be called exactly once after all write_entity() calls.
     */
    void end() noexcept {
        m_w.patch_u32(m_entity_count_offset, m_entity_count);
    }

    /** Expose the underlying buffer view (e.g. to pass to send_message). */
    std::span<const std::byte> view() noexcept {
        return m_w.buffer().view();
    }

    void reset() noexcept {
        m_w.reset();
        m_entity_count = 0;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// WorldStateReader  (client-side / test use)
// ──────────────────────────────────────────────────────────────────────────

struct WorldStateHeader {
    uint32_t packet_type;
    uint32_t frame_idx;
    uint32_t entity_count;
};

struct EntityRecord {
    uint32_t  id;
    glm::vec3 position;
};

class WorldStateReader {
    BinaryReader m_r;
    WorldStateHeader m_header{};

    uint32_t m_spawn_count{ 0 };
    uint32_t m_spawns_read{ 0 };

    uint32_t m_despawn_count{ 0 };
    uint32_t m_despawns_read{ 0 };

    uint32_t m_entities_read{ 0 };

public:
    explicit WorldStateReader(std::span<const std::byte> data) noexcept
        : m_r(data) {}

    /** Read the 12-byte header. Must be called first. */
    WorldStateHeader read_header() noexcept {
        // m_header.packet_type  = m_r.decode<uint32_t>();
        m_header.frame_idx    = m_r.decode<uint32_t>();
        m_header.entity_count = m_r.decode<uint32_t>();

        // spawn count follows immediately
        m_spawn_count  = m_r.decode<uint32_t>();
        return m_header;
    }

    /** Read the next spawn entity id.  Returns 0 when exhausted. */
    bool has_spawn() const noexcept { return m_spawns_read < m_spawn_count; }

    uint32_t read_spawn() noexcept {
        assert(has_spawn());
        ++m_spawns_read;
        uint32_t id = m_r.decode<uint32_t>();
        // if (!has_spawn()) {
        //     // transition to despawns
        //     m_despawn_count = m_r.decode<uint32_t>();
        //     m_phase = Phase::Despawns;
        // }
        return id;
    }

    /** Skip remaining spawns and enter despawn phase. */
    void skip_spawns() noexcept {
        while (has_spawn()) read_spawn();
    }

    bool has_despawn() const noexcept { return m_despawns_read < m_despawn_count; }

    uint32_t read_despawn() noexcept {
        assert(has_despawn());
        ++m_despawns_read;
        uint32_t id = m_r.decode<uint32_t>();
        return id;
    }

    void skip_despawns() noexcept {
        while (has_despawn()) read_despawn();
    }

    bool has_entity() const noexcept {
        return m_entities_read < m_header.entity_count;
    }

    EntityRecord read_entity() noexcept {
        assert(has_entity());
        EntityRecord rec;
        rec.id = m_r.read<uint32_t>();
        m_r.read_bytes(&rec.position.x, 3 * sizeof(float));
        ++m_entities_read;
        return rec;
    }
};

} // namespace tw::serial
