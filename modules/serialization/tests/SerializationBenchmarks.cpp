#include <spdlog/spdlog.h>

#include "WorldState.pb.h"

struct Position {
public:
    float x, y, z;

};

const uint32_t num_messages = 1000;
const uint32_t num_entities = 300;

void protobuf() {
    std::vector<mmo::WorldStateMessage> mesg(num_messages);
    std::vector<Position> positions(num_entities);

    for(int f = 0; f < 100; f++) {
        for(int e = 0; e < num_entities; e++) {
            for(int i = 0; i < num_messages; i++) {
                auto entity = mesg[i].mutable_entities()->Add();
                entity->set_x(positions[e].x);
                entity->set_y(positions[e].y);
                entity->set_z(positions[e].z);
            }
        }

        for(int i = 0; i < num_messages; i++) {
            mesg[i].Clear();
            mesg[i].clear_entities();
        }
    }
}

int main() {
    spdlog::info("Hello world!");

    auto start = std::chrono::high_resolution_clock::now();

    protobuf();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    spdlog::info("Elapsed time: {}", elapsed.count());

    return 0;
}
