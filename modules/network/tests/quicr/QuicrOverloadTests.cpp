/**
 * Testing overloading the listener and how much can it handle.
 */

#include <barrier>
#include <ios>
#include <span>
#include <unordered_map>

#include "Address.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"

using namespace tw::net;
using namespace tw::net::quicr;

int main() {
    // spdlog::set_pattern("[%H:%M:%S] [thread %t] %v");
    const int NUM_CONNECTIONS = 1000;
    std::barrier create_sync_point(NUM_CONNECTIONS + 1);

    std::thread server_thread([&]() {
        auto server_endpoint_r = QuicrEndpoint::create();
        assert(server_endpoint_r);

        auto server_endpoint = std::move(*server_endpoint_r);
        assert(server_endpoint.bind(8100));

        auto listen_r = QuicrConnectionListener::listen(&server_endpoint);
        assert(listen_r);
        auto listen = std::move(*listen_r);

        server_endpoint.assign_listener(&listen);

        create_sync_point.arrive_and_wait();

        struct ConnectionTestSession {
            QuicrConnection *connection;
            bool is_answered;

            std::vector<std::byte> buffer;

            ConnectionTestSession(QuicrConnection *connection)
                : connection(connection), is_answered(false),
                  buffer(1024 * 16) {}
        };

        std::unordered_map<Address, ConnectionTestSession*> connections;
        uint32_t answered_count = 0;
        uint32_t num_connections = 0;

        while(answered_count < NUM_CONNECTIONS) {
            server_endpoint.poll();

            auto new_connection = listen.listen();
            if(new_connection) {
                connections[new_connection->address()] = new ConnectionTestSession(new_connection);
                num_connections++;
                spdlog::warn("Num connections: {}", num_connections);
            }

            for(auto& connection : connections) {
                // assert(!connection.second->is_answered);
                auto read_r = connection.second->connection->read_into(connection.second->buffer);
                assert(read_r);
                if(connection.second->is_answered) {
                    continue;
                }

                std::string mesg(connection.second->buffer.begin(), connection.second->buffer.begin() + *read_r);
                spdlog::error("Received: {}", mesg);
                std::transform(mesg.begin(), mesg.end(), mesg.begin(), ::toupper);

                connection.second->connection->push_stream_frame(std::as_writable_bytes(std::span(mesg)), true);

                connection.second->is_answered = true;
                answered_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::vector<std::thread> client_threads;
    for(int i = 0; i < NUM_CONNECTIONS; i++) {
        client_threads.push_back(std::thread([&](uint32_t idx) {
            create_sync_point.arrive_and_wait();

            auto client_endpoint_r = QuicrEndpoint::create();
            assert(client_endpoint_r);

            auto client_endpoint = std::move(client_endpoint_r.value());

            auto connection_r = client_endpoint.connect(Address{"127.0.0.1", 8100});
            assert(connection_r);
            auto connection = std::move(connection_r.value());

            while(connection->state() != QuicrConnectionState::Established) {
                client_endpoint.poll();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            std::string mesg = "Hello from client " + std::to_string(idx);

            auto bytes = std::as_writable_bytes(std::span(mesg));

            (*connection_r)->push_stream_frame(bytes, true);

            std::string uppercase_mesg = mesg;
            std::transform(uppercase_mesg.begin(), uppercase_mesg.end(), uppercase_mesg.begin(), ::toupper);

            std::vector<std::byte> buffer(1024 * 16);

            while(true) {
                client_endpoint.poll();
                auto read_r = (*connection_r)->read_into(buffer);

                if(*read_r == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }


                std::string received_mesg(buffer.begin(), buffer.begin() + *read_r);
                assert(received_mesg == uppercase_mesg);

                break;
            }
        }, i));
    }

    server_thread.join();
    for(int i = 0; i < NUM_CONNECTIONS; i++) {
        client_threads[i].join();
    }
}
