#include <catch2/catch_test_macros.hpp>

#include "protocol/quicr/QuicrEndpoint.hpp"

using namespace tw::net;
using namespace tw::net::quicr;

TEST_CASE("Endpoint registers new connection with correct ID", "[quicr2]") {
    auto endpoint_r = QuicrEndpoint::create();
    REQUIRE(endpoint_r);
    QuicrEndpoint& server_endpoint = endpoint_r.value();
    REQUIRE(server_endpoint.bind(6972));

    auto client_endpoint_r = QuicrEndpoint::create();
    REQUIRE(client_endpoint_r);
    QuicrEndpoint& client_endpoint = client_endpoint_r.value();

    auto connect_r = client_endpoint.connect(Address{"127.0.0.1", 6972});
    REQUIRE(connect_r);
    QuicrConnection& connection = *connect_r.value();
    REQUIRE(connection.self_id() != 0);
    REQUIRE(connection.peer_id() != 0);

    connection.send_initial_hello();

    client_endpoint.poll();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server_endpoint.poll();

    auto clients = server_endpoint.clients();
    REQUIRE(clients.size() == 2);

    REQUIRE(((clients[0].first == connection.peer_id()) || (clients[1].first == connection.peer_id())));
    REQUIRE(clients[0].second->peer_id() == connection.self_id());
    REQUIRE(clients[1].second->peer_id() == connection.self_id());
}
