#include "catch2/catch_test_macros.hpp"

#include "Address.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrEncoder.hpp"
#include "protocol/quicr/QuicrReliability.hpp"

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

TEST_CASE("When client receives ACK, it won't send the packet again", "[quicr3]") {
    QuicrConnection connection(0, 0, tw::net::Address({}), nullptr);
    connection.send_initial_hello();

    // should contain only the hello frame
    REQUIRE(connection.has_next_datagram() == true);

    auto buffer = connection.pop_datagram();
    QuicrPacket header = QuicrDecoder::decode_packet(buffer);

    std::vector<std::byte> target(1200);
    size_t offset = 0;

    std::vector<uint32_t> acks = { header.packet_number.value() };
    QuicrFrame ack_frame = QuicrFrame::make_ack(std::move(acks));
    QuicrFrame hello_frame = QuicrFrame::make_hello();

    QuicrPacketEncoder encoder(target, offset, QuicrPacketType::Initial, connection);
    encoder
        .encode_frame(ack_frame)
        .encode_frame(hello_frame);

    REQUIRE(connection.has_next_datagram() == false);

    connection.process_datagram(std::span(target).subspan(offset));

    std::this_thread::sleep_for(std::chrono::milliseconds(TW_NET_HELLO_RETRY_INTERVAL_IN_MILLIS));

    REQUIRE(connection.has_next_datagram() == false);
}
