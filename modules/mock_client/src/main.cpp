#include "Address.hpp"
#include "Login.pb.h"
#include "PlayerMove.pb.h"
#include "TcpStream.hpp"
#include "WorldState.pb.h"
#include "messenger/MessageHandler.hpp"
#include "messenger/Messenger.hpp"
#include "runtime/LockStep.hpp"

#include <chrono>
#include <cstdlib>
#include <semaphore>
#include <thread>
#include <glm/glm.hpp>

tw::net::MessageHandler create_messenger(tw::net::Address& address) {
    return tw::net::MessageHandler(address);
}

class MockClient {
    tw::net::MessageHandler m_handler;
    tw::LockStep m_lock_step;

    uint32_t m_frame_idx;
    bool m_is_running;

    uint32_t m_entity_id;
    bool m_is_connected;

    std::binary_semaphore m_connected_semaphore;

    glm::vec3 m_velocity;

public:
    MockClient(tw::net::Address address, const std::string& name) :
        m_handler(create_messenger(address)),
        m_lock_step(20),
        m_is_running(true),
        m_is_connected(false),
        m_connected_semaphore(0)
    {
        m_handler.set_handler<mmo::WorldStateMessage>(
            [&](mmo::WorldStateMessage* mesg) { });

        m_handler.set_handler<mmo::LoginResponse>(
            [&](mmo::LoginResponse* mesg) {
                if(!m_is_connected) {
                    m_is_connected = true;
                    m_entity_id = mesg->entity_id();
                    m_connected_semaphore.release();
                }
            });

        mmo::LoginRequest join_request;
        join_request.set_username(name);
        join_request.set_password("test");

        // m_handler.send(join_request);
    }

    void run() {
        // while(!m_is_connected) {
        //     // m_handler.update();
        //     // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // }
        while(!m_handler.is_connected()) {
            m_handler.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        mmo::PlayerMoveMessage player_move_mesg;

        float value;

        while(m_is_running) {
            if(m_lock_step.wait_for_next_step()) {
                continue;
            }

            m_handler.update();
            value += (float)std::rand() / RAND_MAX;

            m_velocity.x = glm::sin(value);
            m_velocity.z = glm::cos(value);

            m_velocity = glm::normalize(m_velocity);

            player_move_mesg.set_frame_idx(m_frame_idx);
            mmo::PlayerInput* player_input = new mmo::PlayerInput();
            player_input->set_x(m_velocity.x);
            player_input->set_y(0);
            player_input->set_z(m_velocity.z);
            player_move_mesg.set_allocated_input(player_input);
            auto r = m_handler.send(player_move_mesg);
        }
    }
};

int main() {
    const int NUM_CLIENTS = 300;
    tw::net::Address address = {"127.0.0.1", 8101};

    std::vector<std::thread> threads;
    for(uint32_t i = 0; i < NUM_CLIENTS; i++) {
        threads.push_back(std::thread([address, i]() {
            try {
                MockClient client(address, "Player" + std::to_string(i));
                client.run();
            } catch (const std::exception& e) {
                spdlog::error("Client {} failed: {}", i, e.what());
            }
        }));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for(uint32_t i = 0; i < NUM_CLIENTS; i++) {
        threads[i].join();
    }

    return 0;
}
