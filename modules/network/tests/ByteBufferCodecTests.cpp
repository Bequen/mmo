#include "bytebuffer/ByteBuffer.hpp"
#include "bytebuffer/ByteBufferDecoder.hpp"

#include "catch2/catch_test_macros.hpp"

struct DatagramHeader {
    uint32_t type;
    uint64_t secret_key;
};

TEST_CASE("Byte buffer codec test", "[byte_buffer]") {
}
