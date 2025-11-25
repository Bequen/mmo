#include "Address.hpp"
#include "MessageHandler.hpp"
#include "Messenger.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "runtime/LockStep.hpp"
#include <chrono>
#include <cstdlib>
#include <semaphore>
#include <thread>

tw::net::MessageHandler create_messenger(tw::net::Address& address) {
    tw::net::TcpSocket socket;
    socket.connect(address);
    socket.set_non_blocking();

    return tw::net::MessageHandler(tw::net::Messenger(std::move(socket)));
}

class MockClient {
    tw::net::MessageHandler m_handler;
    tw::LockStep m_lock_step;

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
        m_handler.set_handler<EntitySpawnMessage>(
            [&](EntitySpawnMessage* mesg) { });
        
        m_handler.set_handler<EntityDespawnMessage>(
            [&](EntityDespawnMessage* mesg) { });

        m_handler.set_handler<EntityMoveMessage>(
            [&](EntityMoveMessage* mesg) { });

        m_handler.set_handler<PlayerJoinResponseMessage>(
            [&](PlayerJoinResponseMessage* mesg) {
                if(!m_is_connected) {
                    m_is_connected = true;
                    m_entity_id = mesg->entity_id;
                    m_connected_semaphore.release();
                }
            });
        
        PlayerJoinRequestMessage join_request = {
            .name = name 
        };

        m_handler.send(join_request);
    }

    void run() {

        while(!m_is_connected) {
            m_handler.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        PlayerInputMessage player_input_mesg = {
            .frame_idx = 0,
            .entity_id = (uint32_t)m_entity_id,
            .inputs = glm::vec3(0.0f, 0.0f, 0.0f)
        };
        
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

            player_input_mesg.inputs = m_velocity;

            m_handler.send(player_input_mesg);
        }
    }
};

int main() {

    const int NUM_CLIENTS = 16;
    tw::net::Address address = {"127.0.0.1", 8080};

    std::vector<std::thread> threads;
    for(uint32_t i = 0; i < NUM_CLIENTS; i++) {
        threads.push_back(std::thread([address, i]() {
            MockClient client(address, "Player" + std::to_string(i));
            client.run();
        }));
    }

    for(uint32_t i = 0; i < NUM_CLIENTS; i++) {
        threads[i].join();
    }

    return 0;
}
