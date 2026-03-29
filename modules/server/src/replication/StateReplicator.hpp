#pragma once

#include "ClientManager.hpp"
#include "network/NetworkReceiver.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "systems/InterestSystem.hpp"
#include "world/Transform.hpp"

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

namespace tw::net {

/**
 * Decides what to replicate and to whom. Do not change the data of the world.
 */
class StateReplicator {
    const World* m_world;

    PlayerSessionRegistry *m_client_registry;

    NetworkReceiver *m_network;

    const im::InterestSystem *m_interest_manager;

public:
    StateReplicator(
        const World* world,
        PlayerSessionRegistry *client_registry,
        NetworkReceiver *network,
        const im::InterestSystem *interest_manager
    ) :
        m_world(world),
        m_client_registry(client_registry),
        m_network(network),
        m_interest_manager(interest_manager)
    { }

    /**
     * Replicates current state to all the clients.
     */
    void replicate() {
        ZoneScopedN("Replicator");
        for(auto& client : m_client_registry->sessions()) {
            const auto interest = m_interest_manager->get_player_interest(client->session_id);

            // send messages
            if(!interest.entities().empty()) {
                ZoneScopedN("Sending back to client");
                mmo::WorldStateMessage world_state_msg;
                world_state_msg.set_frame_idx(client->last_frame);
                world_state_msg.mutable_entities()->Reserve(interest.entities().size());

                for(const auto& spawn : interest.spawns()) {
                    auto entity = world_state_msg.add_spawns();
                    entity->set_id((uint32_t)spawn);
                }

                {
                    ZoneScopedN("Adding updates to message");
                    for(const auto& update : interest.entities()) {
                        auto entity = world_state_msg.add_entities();
                        entity->set_id((uint32_t)update);

                        auto transform = m_world->registry().get<Transform>(update);
                        entity->set_x(transform.position().x);
                        entity->set_y(transform.position().y);
                        entity->set_z(transform.position().z);
                    }
                }

                for(const auto& despawn : interest.despawns()) {
                    auto entity = world_state_msg.add_despawns();
                    entity->set_id((uint32_t)despawn);
                }

                m_network->send_mesg(client->session_id, world_state_msg);

                // m_outbound_queue->send_fast((PlayerClientId)client.first, world_state_msg);
            } else {
                // spdlog::warn("Entity {} has empty interest", client.first);
            }
        }
    }
};

}
