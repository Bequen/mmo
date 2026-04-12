#include <absl/strings/str_format.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <spdlog/spdlog.h>
#include <sys/uio.h>

#include "WorldState.pb.h"
#include "messages/WorldState_generated.h"

// tw::serial — our custom zero-allocation codec
#include <tw/serial/Serial.hpp>

struct Position {
public:
    float x, y, z;
};

const uint32_t NUM_CLIENTS = 1000;
const uint32_t NUM_ENTITIES = 300;


void protobuf() {
     std::vector<mmo::WorldStateMessage> messages(NUM_CLIENTS * 100); // Simulate 100 frames

     for (int frame = 0; frame < 100; ++frame) {
         for (int i = 0; i < NUM_ENTITIES; ++i) {
             Position pos;
             pos.x = i * 1.5f + 1.0f;
             pos.y = i * 2.0f + 2.0f;
             pos.z = i * 3.0f + 3.0f;

             for (int msg_idx = frame * NUM_CLIENTS; msg_idx < (frame + 1) * NUM_CLIENTS; ++msg_idx) {
                 auto entity = messages[msg_idx].mutable_entities()->Add();
                 entity->set_x(pos.x);
                 entity->set_y(pos.y);
                 entity->set_z(pos.z);
             }
         }
     }
}

void flatbuffers_benchmarks(std::vector<Position> positions) {
    std::vector<flatbuffers::FlatBufferBuilder> builders;
    builders.reserve(NUM_CLIENTS);
    std::vector<std::vector<flatbuffers::Offset<PlayerInfo>>> players(NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        builders.emplace_back(4096);
        players[i].resize(NUM_ENTITIES);
    }

    // reused per frame
    std::vector<iovec> iovecs(NUM_CLIENTS);

    spdlog::info("Running flatbuffers benchmarks...");

    for (int frame = 0; frame < 100; ++frame) {
        for(int position = 0; position < positions.size(); position++) {
            for(int client = 0; client < NUM_CLIENTS; client++) {
                auto& builder = builders[client];

                auto id = position;

                players[client][position] = (
                    CreatePlayerInfo(builder, id, (Vec3*)&positions[position])
                );
            }
        }

        for(int client = 0; client < NUM_CLIENTS; client++ ) {
            auto& builder = builders[client];
            auto players_vec2 = builder.CreateVector(players[client]);
            auto world = CreateWorldState(builder, players_vec2);

            builder.Finish(world);

            builder.Clear();
        }
    }
}

void ours_benchmark(std::vector<Position>& positions) {
    std::vector<std::vector<std::byte>> buffers(NUM_CLIENTS, std::vector<std::byte>(1024*64));

    for(int frame = 0; frame < 100; frame++) {
        for(int position = 0; position < positions.size(); position++) {
            for(int client = 0; client < NUM_CLIENTS; client++) {
                memcpy(&buffers[client][position * (sizeof(uint32_t) + sizeof(Position))], &position, sizeof(int32_t));
                memcpy(&buffers[client][position * (sizeof(uint32_t) + sizeof(Position)) + sizeof(uint32_t)], &positions[position], sizeof(Position));
            }
        }
    }
}

/**
 * tw::serial benchmark — demonstrates the full WorldStateWriter API.
 *
 * Mimics the exact access pattern of StateReplicator::replicate():
 *   - One BinaryBuffer per client, pre-allocated and reused every frame.
 *   - One WorldStateWriter per client per frame.
 *   - Entities written in the inner loop after header/spawns/despawns.
 *   - entity_count patched at the end.
 */
void tw_serial_benchmark(std::vector<Position>& positions) {
    // Pre-allocate one buffer per client
    const std::size_t capacity = 20 + NUM_ENTITIES * 16;
    std::vector<tw::serial::BinaryBuffer> buffers(NUM_CLIENTS);
    for (auto& buf : buffers) {
        buf.reserve(capacity);
    }

    for (int frame = 0; frame < 100; ++frame) {
        // ── Phase 1: write header + empty spawns/despawns ─────────────────
        std::vector<std::size_t> entity_count_offsets(NUM_CLIENTS);

        for (int client = 0; client < NUM_CLIENTS; ++client) {
            auto& buf = buffers[client];
            buf.reset();

            tw::serial::BinaryWriter w(buf);
            // packet_type
            w.encode<uint32_t>(tw::serial::kWorldStatePacketType);
            // frame_idx
            w.encode<uint32_t>(static_cast<uint32_t>(frame));
            // entity_count placeholder
            entity_count_offsets[client] = buf.size();
            w.encode<uint32_t>(0u);
            // no spawns/despawns in this benchmark
            w.encode<uint32_t>(0u); // spawn_count
            w.encode<uint32_t>(0u); // despawn_count
        }

        // ── Phase 2: scatter entity positions (hot path) ──────────────────
        for (int p = 0; p < static_cast<int>(positions.size()); ++p) {
            const uint32_t id = static_cast<uint32_t>(p);
            for (int client = 0; client < NUM_CLIENTS; ++client) {
                buffers[client].append(&id,             sizeof(uint32_t));
                buffers[client].append(&positions[p].x, 3 * sizeof(float));
            }
        }

        // ── Phase 3: patch entity_count ───────────────────────────────────
        for (int client = 0; client < NUM_CLIENTS; ++client) {
            buffers[client].patch_u32(entity_count_offsets[client],
                                      static_cast<uint32_t>(positions.size()));
        }
    }
}

int main() {
    // create test data

    spdlog::info("Running benchmarks...");

    std::vector<Position> positions(NUM_ENTITIES);

    // Protobuf Benchmark
    auto ours_start = std::chrono::high_resolution_clock::now();
    ours_benchmark(positions);
    auto ours_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> ours_elapsed = ours_end - ours_start;
    spdlog::info("Ours elapsed: {} seconds", ours_elapsed.count());

    // Protobuf Benchmark
    auto protobuf_start = std::chrono::high_resolution_clock::now();
    protobuf();
    auto protobuf_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> protobuf_elapsed = protobuf_end - protobuf_start;
    spdlog::info("Protobuf elapsed: {} seconds", protobuf_elapsed.count());

    // FlatBuffers Benchmark
    auto flatbuffers_start = std::chrono::high_resolution_clock::now();
    flatbuffers_benchmarks(positions);
    auto flatbuffers_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> flatbuffers_elapsed = flatbuffers_end - flatbuffers_start;
    spdlog::info("FlatBuffers elapsed: {} seconds", flatbuffers_elapsed.count());

    // tw::serial Benchmark
    auto tw_serial_start = std::chrono::high_resolution_clock::now();
    tw_serial_benchmark(positions);
    auto tw_serial_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tw_serial_elapsed = tw_serial_end - tw_serial_start;
    spdlog::info("tw::serial elapsed: {} seconds", tw_serial_elapsed.count());

    return 0;
}
