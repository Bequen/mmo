#pragma once

#include "network/NetworkReceiver.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "systems/ClientState.hpp"
#include "systems/InterestSystem.hpp"
#include "world/Transform.hpp"

#include <tw/serial/Serial.hpp>

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>
#include <algorithm>
#include <cassert>
#include <complex>

namespace tw::net {

/**
 * Decides what to replicate and to whom.  Does not mutate world state.
 *
 * Per-frame flow:
 *   1. For each session: reset its BinaryBuffer and write the header,
 *      spawns and despawns from the InterestSystem.
 *   2. For each session: iterate its interest set and write entity positions
 *   3. Patch entity_count and send each raw buffer
 *
 * Memory: one BinaryBuffer per session, pre-allocated and reused every frame.
 */
template<im::SpatialBackend Backend>
class StateReplicator {
    const World*                          m_world;
    PlayerSessionRegistry*                m_client_registry;
    NetworkReceiver*                      m_network;
    const im::InterestSystem<Backend>*    m_interest_manager;

    // Per-client writers and their backing buffers reused every frame.
    struct ClientFrame {
        tw::serial::BinaryBuffer buffer;
        // entity_count is appended inline in step 2; we track it separately
        // and patch at step 3.
        uint32_t entity_count{ 0 };
        // Byte offset of the entity_count field inside `buffer`
        std::size_t entity_count_offset{ 0 };
    };

    std::vector<ClientFrame> m_frames;

    // Header(12) + spawn_hdr(4) + despawn_hdr(4) + 512 entities × 16 bytes
    static constexpr std::size_t kInitialCapacity = 20 + 512 * 16;

public:
    StateReplicator(
        const World*                        world,
        PlayerSessionRegistry*              client_registry,
        NetworkReceiver*                    network,
        const im::InterestSystem<Backend>*  interest_manager
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

        // ── 2. Collect interest and write headers / spawns / despawns ─────
        std::vector<const im::ClientState*> client_states(session_count);

        {
            ZoneScopedN("Preparing messages");

            for (std::size_t i = 0; i < session_count; ++i) {
                const auto* session = sessions[i];
                const im::ClientState* state =
                    m_interest_manager->get_player_interest(session->session_id);

                client_states[i] = state;
                if (!state) {
                    // Client not registered in interest system yet
                    continue;
                }

                // Grow buffer to accommodate worst-case entity count
                const std::size_t needed =
                    20
                    + state->spawn().size()   * 4
                    + state->despawn().size() * 4
                    + state->interest().size() * 16;

                auto& frame = m_frames[i];
                frame.buffer.reserve(needed);
                frame.buffer.reset();
                frame.entity_count = 0;

                // ── Write header ────────────────────────────────────────
                // [packet_type:4][frame_idx:4][entity_count:4 (placeholder)]
                // [spawn_count:4][spawn_ids…]
                // [despawn_count:4][despawn_ids…]
                // entity records follow in step 2.

                tw::serial::BinaryWriter w(frame.buffer);

                // packet_type = WORLD_STATE_PACKET (3)
                w.encode<uint32_t>(tw::serial::kWorldStatePacketType);
                // frame_idx
                w.encode<uint32_t>(session->last_frame);
                // entity_count placeholder — remember offset for later patch
                frame.entity_count_offset = frame.buffer.size();
                w.encode<uint32_t>(0u);

                // spawns
                w.encode<uint32_t>(static_cast<uint32_t>(state->spawn().size()));
                for (entt::entity e : state->spawn()) {
                    w.encode<uint32_t>(static_cast<uint32_t>(e));
                }

                // despawns
                w.encode<uint32_t>(static_cast<uint32_t>(state->despawn().size()));
                for (entt::entity e : state->despawn()) {
                    w.encode<uint32_t>(static_cast<uint32_t>(e));
                }
            }
        }

        // ── 2b. Write entity positions per-client (hot path, O(P × K)) ────
        {
            ZoneScopedN("Putting transforms into messages");

            auto view = m_world->registry().view<Transform>();
            view.each([&](entt::entity e, const Transform& t) {
                for (std::size_t i = 0; i < session_count; ++i) {
                    auto& frame = m_frames[i];
                    auto& state = client_states[i];

                    if (state && state->is_interested_in_entity(e)) {
                        const uint32_t raw_id = static_cast<uint32_t>(e);
                        const glm::vec3 pos   = t.position();

                        frame.buffer.append(&raw_id, sizeof(uint32_t));
                        frame.buffer.append(&pos.x,  3 * sizeof(float));
                        ++frame.entity_count;
                    }
                }
            });
        }

        // ── 3. Patch entity_count and dispatch raw buffers ────────────────
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
