#include "TestGenerators.hpp"
#include "bytebuffer/ByteBufferDecoder.hpp"
#include "catch2/catch_test_macros.hpp"
#include "frames/FrameCodec.hpp"
#include "protocol/quicr/QuicrFrameType.hpp"

using namespace tw::net;

TEST_CASE("Encode stream frame", "[frame_encoder]") {
    // std::vector<std::byte> bytes = generate_sequence(8);

    // Frame frame(quicr::FrameType::StreamBase, bytes);

    // ByteBuffer byte_buffer(16);

    // frame::FrameCodec::encode(byte_buffer, frame);

    // REQUIRE(byte_buffer.remaining_read() == 2 * sizeof(uint32_t) + 8);

    // auto frame_type = ByteBufferDecoder<uint32_t>::decode(byte_buffer, 0);

    // REQUIRE(frame_type);
    // REQUIRE(*frame_type == (uint32_t)quicr::FrameType::StreamBase);

    // auto length = ByteBufferDecoder<uint32_t>::decode(byte_buffer, 4);

    // REQUIRE(length);
    // REQUIRE(*length == 8);
}
