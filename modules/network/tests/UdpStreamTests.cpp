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
