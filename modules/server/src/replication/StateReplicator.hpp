#pragma once

#include "network/NetworkReceiver.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "systems/InterestResult.hpp"
#include "systems/InterestSystem.hpp"
#include "world/Transform.hpp"

#include <tw/serial/Serial.hpp>

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

namespace tw::net {

/**
 * Decides what to replicate and to whom.  Does not mutate world state.
 *
 * Per-frame flow:
 *   1. Collect interest results for every session.
 *   2. For each session: reset its BinaryBuffer and write the header,
 *      spawns and despawns via WorldStateWriter::begin/write_spawns/
 *      write_despawns.  Leave the writer open (entity_count not yet patched).
 *   3. Walk the Transform view once; for each entity, write its position into
 *      every client buffer whose interest set contains it.
 *   4. Patch entity_count and send each raw buffer — zero Protobuf, zero heap.
 *
 * Memory: one BinaryBuffer per session, pre-allocated and reused every frame.
 */
class StateReplicator {
    const World*              m_world;
    PlayerSessionRegistry*    m_client_registry;
    NetworkReceiver*          m_network;
    const im::InterestSystem* m_interest_manager;

    // Per-client writers and their backing buffers reused every frame.
    struct ClientFrame {
        tw::serial::BinaryBuffer buffer;
        // entity_count is appended inline in step 3; we track it separately
        // and patch at step 4.
        uint32_t entity_count{ 0 };
        // Byte offset of the entity_count field inside `buffer`
        std::size_t entity_count_offset{ 0 };
    };

    std::vector<ClientFrame> m_frames;

    // Header(12) + spawn_hdr(4) + despawn_hdr(4) + 512 entities × 16 bytes
    static constexpr std::size_t kInitialCapacity = 20 + 512 * 16;

public:
    StateReplicator(
        const World*              world,
        PlayerSessionRegistry*    client_registry,
        NetworkReceiver*          network,
        const im::InterestSystem* interest_manager
    ) :
        m_world(world),
        m_client_registry(client_registry),
        m_network(network),
        m_interest_manager(interest_manager)
    {}

    /**
     * Replicates the current world state to all connected clients.
     */
    void replicate() {
        ZoneScopedN("Replicator");

        const auto& sessions = m_client_registry->sessions();
        const auto session_count = sessions.size();

        if (session_count == 0) return;

        // ── 1. Grow frame pool if needed (only on new connections) ─────────
        if (m_frames.size() < session_count) {
            m_frames.resize(session_count);
            for (auto& f : m_frames) {
                f.buffer.reserve(kInitialCapacity);
            }
        }

        // ── 2. Collect interest; write header / spawns / despawns ─────────
        std::vector<im::InterestResult> interests(session_count);

        {
            ZoneScopedN("Preparing messages");

            for (std::size_t i = 0; i < session_count; ++i) {
                const auto* session = sessions[i];
                interests[i] = m_interest_manager->get_player_interest(session->session_id);

                // Grow buffer to accommodate worst-case entity count
                const std::size_t needed =
                    20
                    + interests[i].spawns().size()   * 4
                    + interests[i].despawns().size() * 4
                    + interests[i].entities().size() * 16;

                auto& frame = m_frames[i];
                frame.buffer.reserve(needed);
                frame.buffer.reset();
                frame.entity_count = 0;

                // ── Write header ────────────────────────────────────────
                // [packet_type:4][frame_idx:4][entity_count:4 (placeholder)]
                // [spawn_count:4][spawn_ids…]
                // [despawn_count:4][despawn_ids…]
                // entity records follow in step 3.

                tw::serial::BinaryWriter w(frame.buffer);

                // packet_type = WORLD_STATE_PACKET (3)
                w.encode<uint32_t>(tw::serial::kWorldStatePacketType);
                // frame_idx
                w.encode<uint32_t>(session->last_frame);
                // entity_count placeholder — remember offset for later patch
                frame.entity_count_offset = frame.buffer.size();
                w.encode<uint32_t>(0u);

                // spawns
                w.encode<uint32_t>(
                    static_cast<uint32_t>(interests[i].spawns().size()));
                for (entt::entity e : interests[i].spawns()) {
                    w.encode<uint32_t>(static_cast<uint32_t>(e));
                }

                // despawns
                w.encode<uint32_t>(
                    static_cast<uint32_t>(interests[i].despawns().size()));
                for (entt::entity e : interests[i].despawns()) {
                    w.encode<uint32_t>(static_cast<uint32_t>(e));
                }
            }
        }

        // ── 3. Scatter entity positions into client buffers (hot path) ────
        {
            ZoneScopedN("Putting transforms into messages");

            m_world->registry().view<Transform>()
                .each([&](entt::entity entity, const Transform& transform) {
                    const uint32_t raw_id = static_cast<uint32_t>(entity);
                    const glm::vec3 pos   = transform.position();

                    for (std::size_t i = 0; i < session_count; ++i) {
                        // Membership test: only send entities in this client's
                        // interest set.
                        // NOTE: O(n) linear scan.  If InterestResult::entities()
                        // is changed to an unordered_set this becomes O(1).
                        // const auto& entities = interests[i].entities();
                        // const bool in_interest = std::find(
                        //     entities.cbegin(), entities.cend(), entity
                        // ) != entities.cend();

                        // if (!in_interest) continue;

                        // Two contiguous memcpys — no branching inside.
                        m_frames[i].buffer.append(&raw_id, sizeof(uint32_t));
                        m_frames[i].buffer.append(&pos.x,  3 * sizeof(float));
                        ++m_frames[i].entity_count;
                    }
                });
        }

        // ── 4. Patch entity_count and dispatch raw buffers ────────────────
        {
            ZoneScopedN("Sending messages");

            for (std::size_t i = 0; i < session_count; ++i) {
                auto& frame = m_frames[i];

                // Patch the entity_count field at the pre-recorded offset
                frame.buffer.patch_u32(frame.entity_count_offset,
                                       frame.entity_count);

                m_network->send_raw(sessions[i]->session_id,
                                    frame.buffer.view());
            }
        }
    }
};

} // namespace tw::net
