#pragma once

#include "network/NetworkReceiver.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "systems/Interest.hpp"
#include "systems/InterestSystem.hpp"
#include "world/Transform.hpp"

#include <tw/serial/Serial.hpp>

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>
#include <cassert>

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
    PlayerSessionRegistry*                m_client_registry;
    NetworkReceiver*                      m_network;

    // Per-client backing buffers reused every frame.
    std::vector<tw::serial::BinaryBuffer> m_frames;

    // Header(12) + spawn_hdr(4) + despawn_hdr(4) + 512 entities × 16 bytes
    static constexpr std::size_t kInitialCapacity = 20 + 512 * 16;

public:
    StateReplicator(
        PlayerSessionRegistry* client_registry,
        NetworkReceiver*       network
    ) :
        m_client_registry(client_registry),
        m_network(network)
    {}

    /**
     * Replicates the current world state for one zone to its connected clients.
     */
    void replicate(const entt::registry& registry, const im::InterestSystem<Backend>* interest_manager) {
        ZoneScopedN("Replicator");

        const auto& sessions = m_client_registry->sessions();
        const std::size_t session_count = sessions.size();

        if (session_count == 0) return;

        // ── Grow buffer pool if needed (only on new connections) ──────────
        if (m_frames.size() < session_count) {
            m_frames.resize(session_count);
            for (auto& buf : m_frames)
                buf.reserve(kInitialCapacity);
        }

        // Build one WorldStateWriter per client referencing the pre-allocated
        // buffer.  reserve() is called before this loop so no reallocation
        // occurs and the buffer references inside the writers stay valid.
        std::vector<tw::serial::WorldStateWriter> writers;
        writers.reserve(session_count);
        for (std::size_t i = 0; i < session_count; ++i)
            writers.emplace_back(m_frames[i]);

        std::vector<const im::Interest*> client_states(session_count);

        // ── 1. Write headers / spawns / despawns ──────────────────────────
        {
            ZoneScopedN("Preparing messages");

            for (std::size_t i = 0; i < session_count; ++i) {
                const auto* session = sessions[i];
                const im::Interest* state =
                    interest_manager->get_interest(session->session_id);

                client_states[i] = state;
                if (!state) continue;

                const std::size_t needed =
                    20
                    + state->spawn().size()    * 4
                    + state->despawn().size()  * 4
                    + state->interest().size() * 16;

                m_frames[i].reserve(needed);
                writers[i].reset();
                writers[i].begin(session->last_frame);
                writers[i].write_spawns(state->spawn());
                writers[i].write_despawns(state->despawn());
            }
        }

        // ── 2. Write entity positions (hot path, O(entities × sessions)) ──
        {
            ZoneScopedN("Putting transforms into messages");

            auto view = registry.view<Transform>();
            view.each([&](entt::entity e, const Transform& t) {
                for (std::size_t i = 0; i < session_count; ++i) {
                    if (client_states[i] && client_states[i]->is_interested_in_entity(e))
                        writers[i].write_entity(e, t.position());
                }
            });
        }

        // ── 3. Finalise and dispatch ───────────────────────────────────────
        {
            ZoneScopedN("Sending messages");

            for (std::size_t i = 0; i < session_count; ++i) {
                spdlog::info("Sending");
                if (!client_states[i]) continue;
                writers[i].end();
                m_network->send_raw(sessions[i]->session_id, writers[i].view());
            }
        }
    }
};

} // namespace tw::net
