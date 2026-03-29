#include <catch2/catch_test_macros.hpp>

// #include "io/Read.hpp"
// #include "messenger/Messenger.hpp"

// class MockReader : public Read<std::byte> {
//     size_t m_cursor;
//     std::string m_content;

// public:
//     MockReader(const std::string& content) :
//         m_cursor(0),
//         m_content(content) {

//     }

//     size_t read(std::span<std::byte> data) override {
//         size_t read_len = std::min(data.size(), m_content.size() - m_cursor);
//         if(read_len == 0) {
//             return 0;
//         }

//         std::copy(m_content.begin() + m_cursor, m_content.begin() + m_cursor + read_len, data.begin());
//         m_cursor += read_len;
//         return read_len;
//     }
// };

// class MockWriter : public Write<std::byte> {

// public:
//     MockWriter() {

//     }

//     size_t write(std::span<const std::byte> data) override {
//     }
// };

// TEST_CASE("Test01", "[Messenger_Test]") {
//     MockReader reader("0Hello, World!");
//     MockWriter writer;
//     tw::net::Messenger messenger(&writer, &reader);

//     REQUIRE(messenger.peek() == '0');
// }
