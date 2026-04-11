#include "catch2/catch_test_macros.hpp"

#include "Address.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEncoder.hpp"
#include "protocol/quicr/QuicrReliability.hpp"
#include <barrier>
#include <span>

using namespace tw::net::quicr;

TEST_CASE("Client begins with Hello datagram", "[quicr2]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);
    connection.send_initial_hello();

    // should contain only the hello frame
    REQUIRE(connection.has_next_datagram() == true);
    auto buffer = connection.pop_datagram();

    QuicrPacket header = QuicrDecoder::decode_packet(buffer);

    REQUIRE(header.type == QuicrPacketType::Initial);

    REQUIRE(header.destination_id == connection.peer_id());
    REQUIRE(header.local_id == connection.self_id());

    REQUIRE(header.frames.size() == 1);

    REQUIRE(header.frames[0].type == FrameType::Hello);

    REQUIRE(connection.has_next_datagram() == false);
}

TEST_CASE("Client wants to resend the Hello", "[quicr2]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);
    connection.send_initial_hello();

    // should contain only the hello frame
    REQUIRE(connection.has_next_datagram() == true);

    auto buffer = connection.pop_datagram();
    QuicrPacket header = QuicrDecoder::decode_packet(buffer);

    REQUIRE(connection.has_next_datagram() == false);

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    REQUIRE(connection.has_next_datagram() == true);

    buffer = connection.pop_datagram();
    header = QuicrDecoder::decode_packet(buffer);

    REQUIRE(header.type == QuicrPacketType::Initial);

    REQUIRE(header.destination_id == connection.peer_id());
    REQUIRE(header.local_id == connection.self_id());

    REQUIRE(header.frames.size() == 1);

    REQUIRE(header.frames[0].type == FrameType::Hello);

    REQUIRE(connection.has_next_datagram() == false);
}

TEST_CASE("Closed connection will setup connection IDs after Hello", "[quicr2]") {

}

TEST_CASE("Connection reacts to Hello with ACK & Hello", "[quicr2]") {
    QuicrConnection client(0, 0, tw::net::Address({}), nullptr);
    client.send_initial_hello();

    auto hello = client.pop_datagram();

    QuicrConnection server(0, 0, tw::net::Address({}), nullptr);

    server.process_datagram(hello);

    REQUIRE(server.has_next_datagram() == true);

    auto dgram = server.pop_datagram();

    QuicrPacket packet = QuicrDecoder::decode_packet(dgram);

    REQUIRE(packet.type == QuicrPacketType::Initial);

    REQUIRE(packet.destination_id == server.peer_id());
    REQUIRE(packet.local_id == server.self_id());

    REQUIRE(packet.frames.size() == 2);

    REQUIRE(std::any_of(packet.frames.begin(), packet.frames.end(), [](const QuicrFrame& f) { return f.type == FrameType::Ack; }));
    REQUIRE(std::any_of(packet.frames.begin(), packet.frames.end(), [](const QuicrFrame& f) { return f.type == FrameType::Hello; }));
}

TEST_CASE("Both connections have correct IDs after Initial exchange", "[quicr2]") {
    QuicrConnection client(0, 0, tw::net::Address({}), nullptr);
    client.send_initial_hello();

    auto client_hello = client.pop_datagram();

    QuicrConnection server(0, 0, tw::net::Address({}), nullptr);

    server.process_datagram(client_hello);

    auto server_hello = server.pop_datagram();

    client.process_datagram(server_hello);

    REQUIRE(client.self_id() == server.peer_id());
    REQUIRE(client.peer_id() == server.self_id());
}

TEST_CASE("When client receives ACK, it won't send the packet again", "[quicr2]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);
    connection.send_initial_hello();

    // should contain only the hello frame
    REQUIRE(connection.has_next_datagram() == true);

    auto buffer = connection.pop_datagram();
    QuicrPacket header = QuicrDecoder::decode_packet(buffer);

    REQUIRE(connection.has_next_datagram() == false);

    std::vector<std::byte> target(1200);
    size_t offset = 0;

    std::vector<uint32_t> acks = { header.packet_number.value() };
    QuicrFrame hello_frame = QuicrFrame::make_hello();

    QuicrPacketEncoder encoder(target, offset, QuicrPacketType::Initial, 0, connection);
    encoder
        .encode_ack_frame(acks);


    connection.process_datagram(std::span(target).subspan(0, encoder.size()));

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    REQUIRE(connection.has_next_datagram() == false);
}

TEST_CASE("Connection don't send ACK when packet has no reliable frames", "[quicr3]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);

    auto buffer = connection.pop_datagram();
    QuicrPacket header = QuicrDecoder::decode_packet(buffer);

    std::vector<std::byte> target(1200);
    size_t offset = 0;

    std::string message = "Hello world";

    QuicrPacketEncoder encoder(target, offset, QuicrPacketType::Initial, 0, connection);
    encoder
        .encode_stream_frame(std::as_writable_bytes(std::span(message)), false);

    REQUIRE(connection.has_next_datagram() == false);

    connection.process_datagram(std::span(target).subspan(0, encoder.size()));

    REQUIRE(connection.has_next_datagram() == false);

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    REQUIRE(connection.has_next_datagram() == false);
}

TEST_CASE("Connection sends ACK when the packet has reliable frames", "[quicr3]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);

    auto buffer = connection.pop_datagram();
    QuicrPacket header = QuicrDecoder::decode_packet(buffer);

    std::vector<std::byte> target(1200);
    size_t offset = 0;

    std::string message = "Hello world";

    QuicrPacketEncoder encoder(target, offset, QuicrPacketType::Initial, 0, connection);
    encoder
        .encode_stream_frame(std::as_writable_bytes(std::span(message)), true);

    REQUIRE(connection.has_next_datagram() == false);

    connection.process_datagram(std::span(target).subspan(0, encoder.size()));

    REQUIRE(connection.has_next_datagram() == true);

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    REQUIRE(connection.has_next_datagram() == true);
}

TEST_CASE("Connection applies to ACK to all packets that sent the frame", "[quicr2]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);
    connection.send_initial_hello();

    auto dgram1 = connection.pop_datagram();
    auto packet1 = QuicrDecoder::decode_packet(dgram1);

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    auto dgram2 = connection.pop_datagram();
    auto packet2 = QuicrDecoder::decode_packet(dgram2);

    REQUIRE(packet1.packet_number.value() != packet2.packet_number.value());

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    std::vector<std::byte> target(1200);
    size_t offset = 0;
    QuicrConnection connection2(0, 0, tw::net::Address({}), nullptr);

    std::vector<uint32_t> acks = { packet1.packet_number.value() };
    QuicrPacketEncoder encoder(target, offset, QuicrPacketType::Initial, 0, connection2);
    encoder
        .encode_ack_frame(acks);

    connection.process_datagram(std::span(target).subspan(0, encoder.size()));

    REQUIRE(!connection.has_next_datagram());
}

TEST_CASE("Connection can be established", "[quicr2]") {
    std::barrier create_sync_point(2);
    std::barrier send_sync_point(2);
    std::barrier client_send_sync_point(2);
    std::string mesg = "Hello world";
    std::string client_msg = "Client hello";

    std::thread server_thread([&]() {
        auto endpoint_r = QuicrEndpoint::create();

        REQUIRE(endpoint_r);

        auto endpoint = std::move(endpoint_r.value());

        REQUIRE(endpoint.bind(6971));

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
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        endpoint.poll();

        spdlog::info("Connection established with peer id: 0x{:x}", connection->peer_id());

        // write whole message
        auto bytes = std::as_writable_bytes(std::span(mesg.begin(), mesg.end()));
        auto r = connection->send_message(bytes, true);
        if(!r) {
            spdlog::error("Failed to write to connection");
        }

        REQUIRE(r);

        endpoint.poll();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        send_sync_point.arrive_and_wait();
        client_send_sync_point.arrive_and_wait();

        endpoint.poll();
    });

    std::thread client_thread([&]() {
        create_sync_point.arrive_and_wait();

        auto endpoint_r = QuicrEndpoint::create();
        REQUIRE(endpoint_r);
        auto endpoint = std::move(*endpoint_r);

        auto connection_result = endpoint.connect(tw::net::Address {"127.0.0.1", 6971}); // QuicrConnection::connect(Address{"127.0.0.1", 6970});
        REQUIRE(connection_result);

        auto conn = std::move(*connection_result);

        spdlog::info("Client ID: {}", conn->self_id());

        while(conn->state() != QuicrConnectionState::Established) {
            endpoint.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        endpoint.poll();

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
        conn->send_message(bytes, true);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        client_send_sync_point.arrive_and_wait();
    });

    client_thread.join();
    server_thread.join();
}

TEST_CASE("Send large datagram", "[quicr2]") {
    std::barrier create_sync_point(2);
    std::barrier send_sync_point(2);
    std::barrier client_send_sync_point(2);
    std::string mesg = std::string(2000, 'a');

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
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        endpoint.poll();

        spdlog::info("Connection established with peer id: 0x{:x}", connection->peer_id());

        // write whole message
        auto bytes = std::as_writable_bytes(std::span(mesg.begin(), mesg.end()));
        auto r = connection->send_message(bytes, true);
        if(!r) {
            spdlog::error("Failed to write to connection");
        }

        REQUIRE(r);

        endpoint.poll();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        send_sync_point.arrive_and_wait();
        client_send_sync_point.arrive_and_wait();

        endpoint.poll();
    });

    std::thread client_thread([&]() {
        create_sync_point.arrive_and_wait();

        auto endpoint_r = QuicrEndpoint::create();
        REQUIRE(endpoint_r);
        auto endpoint = std::move(*endpoint_r);

        spdlog::info("Connecting");
        auto connection_result = endpoint.connect(tw::net::Address {"127.0.0.1", 6970}); // QuicrConnection::connect(Address{"127.0.0.1", 6970});
        if(!connection_result) {
            spdlog::error("Failed to connect to server: {}", connection_result.error().message());
        }

        auto conn = std::move(*connection_result);

        spdlog::info("Client ID: {}", conn->self_id());

        while(conn->state() != QuicrConnectionState::Established) {
            endpoint.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        endpoint.poll();

        spdlog::info("Connection established");

        send_sync_point.arrive_and_wait();

        endpoint.poll();

        std::string buffer(64 * 1024, '\0');
        spdlog::info("Waiting to receive message from server...");
        auto r = conn->read_into(std::as_writable_bytes(std::span(buffer.data(), buffer.size())));
        if(!r) {
            spdlog::error("Failed to read from connection: {}", r.error().message());
        }

        spdlog::info("Received: [{}], {}", r.value(), buffer.substr(0, r.value()));

        REQUIRE(buffer.substr(0, r.value()) == mesg);

        auto bytes = std::as_writable_bytes(std::span(client_msg.begin(), client_msg.end()));
        conn->send_message(bytes, true);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        client_send_sync_point.arrive_and_wait();
    });

    client_thread.join();
    server_thread.join();
}

TEST_CASE("Sending message through closed connection returns error", "[quicr2]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);

    REQUIRE(connection.state() == QuicrConnectionState::Closed);

    std::string mesg = "Hello world";
    auto bytes = std::as_writable_bytes(std::span(mesg.begin(), mesg.end()));
    auto send_r = connection.send_message(bytes, true);
    REQUIRE(!send_r);

    REQUIRE(send_r.error().type() == QuicrErrorType::ConnectionClosed);
}

TEST_CASE("Frame can close the connection", "[quicr3]") {

}
