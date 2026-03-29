#include <barrier>
#include <catch2/catch_test_macros.hpp>
#include <span>
#include <iostream>

#include "UdpStream.hpp"
#include "Address.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"

TEST_CASE("Start two sockets and send message", "[udp]") {
    std::barrier create_sync_point(2);
    std::barrier send_sync_point(2);

    const std::string message = "Hello, server!";

    std::thread server_thread([&]() {
        auto r = tw::net::UdpStream::bind(tw::net::Address {"127.0.0.1", 6969});
        if(!r.has_value()) {
            std::cout << ("Failed to bind UDP stream: {}") << r.error().message() << std::endl;
        }

        auto flag_result = r->set_non_blocking();

        auto server = std::move(r.value());

        create_sync_point.arrive_and_wait();
        send_sync_point.arrive_and_wait();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::vector<std::byte> buffer(1024);
        tw::net::Address from {{}, 0};
        auto read_result = server.read_into(std::span{buffer.data(), buffer.size()}, &from);

        if(!read_result.has_value()) {
            spdlog::error("Failed to read from UDP stream: {}", read_result.error().message());
        }

        // convert the buffer to a string
        std::string mesg(buffer.data(), buffer.data() + read_result.value());
        REQUIRE(mesg == message);
    });

    std::thread client_thread([&]() {
        create_sync_point.arrive_and_wait();

        auto connect_result = tw::net::UdpStream::to(tw::net::Address{"127.0.0.1", 6969});
        if(!connect_result.has_value()) {
            spdlog::error("Failed to bind UDP stream: {}", connect_result.error().mesg());
        }

        auto client = std::move(connect_result.value());

        std::string mesg = message;
        auto write_result = client.write(std::as_writable_bytes(std::span(mesg.begin(), mesg.end())));

        if(!write_result.has_value() && write_result.value() == mesg.length()) {
            spdlog::error("Failed to write to UDP stream: {}", write_result.error().message());
        }

        send_sync_point.arrive_and_wait();
    });

    server_thread.join();
    client_thread.join();
}

using namespace tw::net;
using namespace tw::net::quicr;

TEST_CASE("UDP Connection", "[quicr]") {
    std::barrier create_sync_point(2);
    std::barrier send_sync_point(2);
    std::barrier client_send_sync_point(2);
    std::string mesg = "Hello world";
    std::string client_msg = "Client hello";

    std::thread server_thread([&]() {
        auto endpoint_r = QuicrEndpoint::create();
        REQUIRE(endpoint_r);
        auto endpoint = std::move(endpoint_r.value());
        REQUIRE(endpoint.bind(6970));

        auto listener_r = QuicrConnectionListener::listen(&endpoint);
        REQUIRE(listener_r);
        auto listener = std::move(*listener_r);
        endpoint.assign_listener(&listener);

        create_sync_point.arrive_and_wait();

        QuicrConnection* connection = nullptr;

        // wait for connection
        while(connection == nullptr) {
            endpoint.poll();
            connection = listener.listen();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        spdlog::info("Connection established with peer id: 0x{:x}", connection->peer_id());

        // write whole message
        auto bytes = std::as_writable_bytes(std::span(mesg.begin(), mesg.end()));
        auto r = connection->push_stream_frame(bytes, true);
        if(!r) {
            spdlog::error("Failed to write to connection");
        }

        REQUIRE(r);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        send_sync_point.arrive_and_wait();
        client_send_sync_point.arrive_and_wait();

        endpoint.poll();

        std::string buffer(1024, '\0');
        spdlog::info("Waiting to receive message from server...");
        auto read_r = connection->read_into(std::as_writable_bytes(std::span(buffer.data(), buffer.size())));
        if(!read_r) {
            spdlog::error("Failed to read from connection: {}", read_r.error().message());
        }

        spdlog::info("Received: [{}], {}", read_r.value(), buffer.substr(0, read_r.value()));

        REQUIRE(buffer.substr(0, read_r.value()) == client_msg);
    });

    std::thread client_thread([&]() {
        create_sync_point.arrive_and_wait();

        auto endpoint_r = QuicrEndpoint::create();
        REQUIRE(endpoint_r);
        auto endpoint = std::move(*endpoint_r);

        spdlog::info("Connecting");
        auto connection_result = endpoint.connect(Address {"127.0.0.1", 6970}); // QuicrConnection::connect(Address{"127.0.0.1", 6970});
        if(!connection_result) {
            spdlog::error("Failed to connect to server: {}", connection_result.error().message());
        }

        auto conn = std::move(*connection_result);
        while(conn->state() != QuicrConnectionState::Established) {
            endpoint.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        spdlog::info("Connection established");

        send_sync_point.arrive_and_wait();

        endpoint.poll();

        std::string buffer(1024, '\0');
        spdlog::info("Waiting to receive message from server...");
        auto r = conn->read_into(std::as_writable_bytes(std::span(buffer.data(), buffer.size())));
        if(!r) {
            spdlog::error("Failed to read from connection: {}", r.error().message());
        }

        spdlog::info("Received: [{}], {}", r.value(), buffer.substr(0, r.value()));

        REQUIRE(buffer.substr(0, r.value()) == mesg);

        auto bytes = std::as_writable_bytes(std::span(client_msg.begin(), client_msg.end()));
        conn->push_stream_frame(bytes, true);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        client_send_sync_point.arrive_and_wait();
    });

    client_thread.join();
    server_thread.join();
}
