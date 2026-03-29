#include "TestGenerators.hpp"
#include "bytebuffer/ByteBuffer.hpp"
#include "bytebuffer/ByteBufferDecoder.hpp"

#include "catch2/catch_test_macros.hpp"

TEST_CASE("Byte buffer insert", "[byte_buffer]") {
    std::vector<std::byte> bytes = generate_sequence(8);
    std::vector<std::byte> storage(12);

    tw::net::RingByteBuffer byte_buffer(storage, false);
    REQUIRE(byte_buffer.remaining_write() == 12);
    REQUIRE(byte_buffer.remaining_read() == 0);

    size_t written = byte_buffer.write_bytes(bytes);

    REQUIRE(byte_buffer.remaining_write() == 4);
    REQUIRE(byte_buffer.remaining_read() == 8);
    REQUIRE(written == 8);
}

TEST_CASE("Byte buffer pop", "[byte_buffer]") {
    std::vector<std::byte> bytes = generate_sequence(8);
    std::vector<std::byte> storage(12);

    tw::net::RingByteBuffer byte_buffer(storage, false);
    size_t written = byte_buffer.write_bytes(bytes);

    std::vector<std::byte> popped(6);
    size_t read = byte_buffer.pop_bytes(popped.data(), 6);

    REQUIRE(read == 6);

    REQUIRE(popped[0] == std::byte{1});
    REQUIRE(popped[1] == std::byte{2});
    REQUIRE(popped[2] == std::byte{3});
    REQUIRE(popped[3] == std::byte{4});
    REQUIRE(popped[4] == std::byte{5});
    REQUIRE(popped[5] == std::byte{6});

    REQUIRE(byte_buffer.remaining_read() == 2);
    REQUIRE(byte_buffer.remaining_write() == 10);
}

TEST_CASE("Byte buffer decoder", "[byte_buffer]") {
    std::vector<std::byte> bytes = generate_sequence(8);
    std::vector<std::byte> storage(12);

    tw::net::RingByteBuffer byte_buffer(storage, false);
    size_t written = byte_buffer.write_bytes(bytes);
}
