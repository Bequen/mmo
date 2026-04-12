#include "InterestSystem.hpp"
#include "systems/InterestResult.hpp"
#include "world/Transform.hpp"
#include <algorithm>

namespace tw::net::im {

InterestSystem::InterestSystem(const World* world, const PlayerSessionRegistry *registry)
    : m_world(world),
    m_session_registry(registry)
{ }

void InterestSystem::update() {
    ZoneScopedN("InterestSystem::update");
    for(auto& [client_id, state] : m_client_entities) {
        auto entity = state.entity();
        if(entity == entt::null) {
            continue;
        }

        auto* transform = m_world->registry().try_get<Transform>(entity);
        if(transform == nullptr) {
            // continue with empty interest
            continue;
        }

        // filter entities that are of interest
        std::set<entt::entity> interest;
        m_world->registry()
            .view<Transform>()
            .each([&interest, transform, entity](const auto e, const Transform& pos) {
                if (pos.is_closer_than(*transform, 1000.0f)) {
                    interest.insert(e);
                }
            });

        std::vector<entt::entity> spawn;
        std::set_difference(interest.begin(), interest.end(),
            state.interest().begin(), state.interest().end(),
            std::back_inserter(spawn));

        std::vector<entt::entity> despawn;
        std::set_difference(state.interest().begin(), state.interest().end(),
            interest.begin(), interest.end(),
            std::back_inserter(despawn));

        state.interest().swap(interest);
        state.despawn().swap(despawn);
        state.spawn().swap(spawn);
    }
}

const InterestResult InterestSystem::get_player_interest(PlayerClientId client_id) const {
    ZoneScopedN("InterestSystem::get_player_interest");
    InterestResult result;

    auto state = get_client_state(client_id);
    if(state == nullptr) {
        return result;
    }

    result.set_update(std::vector<entt::entity>(state->interest().begin(), state->interest().end()));
    result.set_despawn(state->despawn());
    result.set_spawn(state->spawn());

    return result;
}

}
