/**
 * Testing overloading the listener and how much can it handle.
 */

#include <barrier>
#include <ios>
#include <span>
#include <unordered_map>
#include <tracy/Tracy.hpp>

#include "Address.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"

using namespace tw::net;
using namespace tw::net::quicr;

std::atomic<bool> is_stopped(false);

void got_signal(int) {
    is_stopped.store(true);
}

void register_signal_handler() {
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);
}

int main() {
    register_signal_handler();
    // spdlog::set_pattern("[%H:%M:%S] [thread %t] %v");
    const int NUM_CONNECTIONS = 500;
    std::thread server_thread([&]() {
        auto server_endpoint_r = QuicrEndpoint::create();
        assert(server_endpoint_r);

        auto server_endpoint = std::move(*server_endpoint_r);
        assert(server_endpoint.bind(8100));

        auto listen_r = QuicrConnectionListener::listen(&server_endpoint);
        assert(listen_r);
        auto listen = std::move(*listen_r);

        server_endpoint.assign_listener(&listen);


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
            if(is_stopped) break;
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

                connection.second->connection->send_message(std::as_writable_bytes(std::span(mesg)), true);

                connection.second->is_answered = true;
                answered_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        spdlog::warn("DONE: got all answers");
    });

    std::vector<std::unique_ptr<QuicrEndpoint>> endpoints(NUM_CONNECTIONS);
    std::vector<QuicrConnection*> connections(NUM_CONNECTIONS);
    std::vector<bool> established_counts(NUM_CONNECTIONS, false);

    for(int i = 0; i < NUM_CONNECTIONS; i++) {
        ZoneScoped;
        endpoints[i] = std::make_unique<QuicrEndpoint>(QuicrEndpoint::create().value());

        connections[i] = endpoints[i]->connect(Address{"127.0.0.1", 8100}).value();
    }

    std::atomic<uint32_t> established_count(0);
    spdlog::info("Starting overload test with {} connections", NUM_CONNECTIONS);

    while(!is_stopped && established_count.load() < NUM_CONNECTIONS) {
        for(int i = 0; i < NUM_CONNECTIONS; i++) {
            {
                endpoints[i]->poll();
            }
            if(!established_counts[i] && connections[i]->state() == QuicrConnectionState::Established) {
                established_counts[i] = true;
                established_count++;
            }
        }
    }

    // for(int i = 0; i < NUM_CONNECTIONS; i++) {
    //     client_threads.push_back(std::thread([&](uint32_t idx) {
    //         ZoneScoped;
    //         create_sync_point.arrive_and_wait();

    //         auto client_endpoint_r = QuicrEndpoint::create();
    //         assert(client_endpoint_r);

    //         auto client_endpoint = std::move(client_endpoint_r.value());

    //         auto connection_r = client_endpoint.connect(Address{"127.0.0.1", 8100});
    //         assert(connection_r);
    //         auto connection = std::move(connection_r.value());

    //         while(connection->state() != QuicrConnectionState::Established) {
    //             if(is_stopped) break;
    //             client_endpoint.poll();
    //             std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //         }

    //         std::string mesg = "Hello from client " + std::to_string(idx) + std::string(1500, 'a');

    //         auto bytes = std::as_writable_bytes(std::span(mesg));

    //         (*connection_r)->push_stream_frame(bytes, true);

    //         std::string uppercase_mesg = mesg;
    //         std::transform(uppercase_mesg.begin(), uppercase_mesg.end(), uppercase_mesg.begin(), ::toupper);

    //         std::vector<std::byte> buffer(1024 * 16);

    //         while(true) {
    //             if(is_stopped) break;
    //             client_endpoint.poll();
    //             auto read_r = (*connection_r)->read_into(buffer);

    //             if(*read_r == 0) {
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //                 continue;
    //             }


    //             std::string received_mesg(buffer.begin(), buffer.begin() + *read_r);
    //             assert(received_mesg == uppercase_mesg);

    //             break;
    //         }
    //     }, i));
    // }

    server_thread.join();
    return 0;
}
