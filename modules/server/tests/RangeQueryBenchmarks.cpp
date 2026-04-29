

#include "entt/entt.hpp"
#include "interest_management/FixedGrid.hpp"
#include "interest_management/SpatialHashGrid.hpp"
#include <chrono>
#include <common/TracyQueue.hpp>
#include <cstdint>
#include <random>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

// #include "Quadtree.h"

void* operator new(std::size_t count) {
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    TracyFree(ptr);
    free(ptr);
}

const long SIZE = 4096*4;
const long VIEW = 100;

struct Position {
    float x, y, z;

    Position(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Velocity {
    float x, y, z;
};

void generate_points(entt::registry& registry, uint32_t num_entities) {
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> pos_dist(-SIZE, SIZE);
    std::uniform_real_distribution<float> vel_dist(-5.0f, 5.0f);

    for (uint32_t i = 0; i < num_entities; ++i) {
        auto entity = registry.create();
        registry.emplace<Position>(entity, pos_dist(rng), pos_dist(rng), pos_dist(rng));
        registry.emplace<Velocity>(entity, vel_dist(rng), vel_dist(rng), vel_dist(rng));
    }
}

float clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
}

void move_points(entt::registry& registry) {
    for (auto entity : registry.view<Position, Velocity>()) {
        auto& pos = registry.get<Position>(entity);
        auto& vel = registry.get<Velocity>(entity);
        pos.x += vel.x;
        pos.y += vel.y;
        pos.z += vel.z;

        pos.x = clamp(pos.x, -SIZE, SIZE);
        pos.y = clamp(pos.y, -SIZE, SIZE);
        pos.z = clamp(pos.z, -SIZE, SIZE);
    }
}

std::vector<glm::vec3> generate_query_points(uint32_t num_points) {
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(-SIZE, SIZE);
    std::vector<glm::vec3> points;
    for (uint32_t i = 0; i < num_points; ++i) {
        points.emplace_back(dist(rng), dist(rng), dist(rng));
    }
    return points;
}

const uint32_t ENTITY_COUNT = 100000;
const uint32_t FRAME_COUNT = 100;

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

void hash_grid(entt::registry& registry, const std::vector<glm::vec3>& query_points) {
    tw::net::im::SpatialHashGrid<VIEW, VIEW> hash_grid;
    ZoneScopedN("HashGrid");

    TimePoint start = Clock::now();

    for(int frame = 0; frame < FRAME_COUNT; frame++) {
    { ZoneScopedN("move"); move_points(registry); }
    hash_grid.begin_frame();
    { ZoneScopedN("insert");
        for (auto entity : registry.view<Position>()) {
            auto [x, y, z] = registry.get<Position>(entity);
            hash_grid.insert(entity, glm::vec3(x, y, z));
        }
    }
    { ZoneScopedN("query");
        for (const auto& pos : query_points) {
            std::vector<entt::entity> out_entities;
            hash_grid.query_neighbors(pos, out_entities);
        }
    }
    }

    TimePoint end = Clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("HashGrid Elapsed time: {}s", elapsed.count());
}

void fixed_grid(entt::registry& registry, const std::vector<glm::vec3>& query_points) {
    tw::net::im::FixedGrid<VIEW, VIEW> fixed_grid(-10000, 10000, -10000, 10000);
    ZoneScopedN("FixedGrid");

    auto start = Clock::now();

    for(int frame = 0; frame < FRAME_COUNT; frame++) {
        { ZoneScopedN("move"); move_points(registry); }
        fixed_grid.begin_frame();
        { ZoneScopedN("insert");
            for (auto entity : registry.view<Position>()) {
                auto [x, y, z] = registry.get<Position>(entity);
                fixed_grid.insert(entity, glm::vec3(x, y, z));
            }
        }
        { ZoneScopedN("query");
            for (const auto& pos : query_points) {
                std::vector<entt::entity> out_entities;
                fixed_grid.query_neighbors(pos, out_entities);
            }
        }
    }

    auto end = Clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("FixedGrid Elapsed time: {}s", elapsed.count());
}

void naive(entt::registry& registry, const std::vector<glm::vec3>& query_points) {
    auto start = Clock::now();
    ZoneScopedN("Naive");

    for(int frame = 0; frame < FRAME_COUNT; frame++) {
        { ZoneScopedN("move"); move_points(registry); }
        { ZoneScopedN("query");
            for(const auto& player : query_points) {
                std::vector<entt::entity> out_entities;
                for(const auto& position : registry.view<Position>()) {
                    auto [x, y, z] = registry.get<Position>(position);
                    Position diff(player.x - x, player.y - y, 0.0f);

                    if(diff.x * diff.x + diff.y * diff.y < VIEW * VIEW) {
                        out_entities.push_back((entt::entity)position);
                    }
                }
            }
        }
    }

    auto end = Clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("Naive Elapsed time: {}ms", elapsed.count());
}

// void quadtree_query(entt::registry& registry, const std::vector<glm::vec3>& query_points) {
//     ZoneScopedN("quadtree_query");
//     struct Node
//     {
//         quadtree::Box<float> box;
//         uint32_t id;
//     };

//     auto getBox = [](Node* node)
//     {
//         return node->box;
//     };

//     auto box = quadtree::Box((float)-SIZE, (float)-SIZE, (float)2*SIZE, (float)2*SIZE);
//     std::vector<Node> nodes(ENTITY_COUNT);

//     auto start = Clock::now();

//     for(int frame = 0; frame < FRAME_COUNT; frame++) {
//         quadtree::Quadtree<Node*, decltype(getBox)> quad_tree(box, getBox);
//         { ZoneScopedN("move");
//             move_points(registry); }
//         { ZoneScopedN("insert");
//             for (auto entity : registry.view<Position>()) {
//                 auto [x, y, z] = registry.get<Position>(entity);
//                 auto node = &nodes[(uint32_t)entity];
//                 node->box = quadtree::Box(x, y, 0.0f, 0.0f);
//                 node->id = (uint32_t)entity;
//                 quad_tree.add(node);
//             }
//         }
//         { ZoneScopedN("query");
//             for (const auto& pos : query_points) {
//                 quad_tree.query(quadtree::Box(pos.x - 10.0f, pos.y - 10.0f, 20.0f, 20.0f));
//             }
//         }
//     }

//     auto end = Clock::now();
//     auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//     spdlog::info("QuadTree Elapsed time: {}ms", elapsed.count());
// }

int main() {
    entt::registry registry;

    generate_points(registry, ENTITY_COUNT);

    auto query_points = generate_query_points(1000);

    naive(registry, query_points);
    // quadtree_query(registry, query_points);
    fixed_grid(registry, query_points);
    hash_grid(registry, query_points);
}
