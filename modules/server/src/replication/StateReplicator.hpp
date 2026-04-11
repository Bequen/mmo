#pragma once

#include "WorldState.pb.h"
#include "network/NetworkReceiver.hpp"
#include "network/PlayerSessionRegistry.hpp"
#include "systems/InterestResult.hpp"
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

    google::protobuf::Arena arena;

    std::vector<mmo::WorldStateMessage> messages;

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

        // std::span<mmo::WorldStateMessage> messages = message_sender->allocate<mmo::WorldStateMessage>(interests.size());

        std::vector<im::InterestResult> interests(m_client_registry->sessions().size());
        messages.resize(interests.size());
        std::vector<std::vector<std::byte>> buffers(interests.size(), std::vector<std::byte>(1200));

        {
            ZoneScopedN("Preparing messages");
            for(int i = 0; i < m_client_registry->sessions().size(); ++i) {
                interests[i] = m_interest_manager->get_player_interest(m_client_registry->sessions()[i]->session_id);

                messages[i].set_frame_idx(m_client_registry->sessions()[i]->last_frame);
                messages[i].mutable_entities()->Reserve(interests[i].entities().size());

                for(const auto& spawn : interests[i].spawns()) {
                    auto entity = messages[i].add_spawns();
                    entity->set_id((uint32_t)spawn);
                }

                for(const auto& despawn : interests[i].despawns()) {
                    auto entity = messages[i].add_despawns();
                    entity->set_id((uint32_t)despawn);
                }
            }
        }

        {
            ZoneScopedN("Putting transforms into messages");
            m_world->registry().view<Transform>()
                .each([&](entt::entity entity, const Transform& transform) {
                    uint32_t id = (uint32_t)entity;
                    for(int i = 0; i < interests.size(); i++) {
                        auto& buf = buffers[i];
                        buf[0] = (std::byte(id & 0xFF));
                        buf[1] = (std::byte((id >> 8) & 0xFF));
                        buf[2] = (std::byte((id >> 16) & 0xFF));
                        buf[3] = (std::byte((id >> 24) & 0xFF));

                        auto pos = transform.position();
                        memcpy(&buf[4], (void*)&pos, 3 * sizeof(float));

                        // mmo::EntityPosition* world_entity = nullptr;
                        // world_entity = messages[i].mutable_entities()->Add();

                        // world_entity->set_id((uint32_t)entity);

                        // auto pos = transform.position();
                        // world_entity->set_x(pos.x);
                        // world_entity->set_y(pos.y);
                        // world_entity->set_z(pos.z);
                    }
                });
        }

        {
            ZoneScopedN("Sending messages");
            for(int i = 0; i < interests.size(); i++) {
                m_network->send_mesg(m_client_registry->sessions()[i]->session_id, messages[i]);
                messages[i].clear_entities();
                messages[i].clear_despawns();
                messages[i].clear_spawns();
                messages[i].clear_frame_idx();
            }
        }

        #if 0
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
        #endif
    }
};

}
