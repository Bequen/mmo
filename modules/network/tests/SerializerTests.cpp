#include <catch2/catch_test_macros.hpp>
#include <print>
#include "Serializers.hpp"

struct TestMessage1 {
    uint32_t a, b, c, d;
};

template<>
class tw::net::Serializer<TestMessage1> {
public:
    static bool serialize(Serialization& buffer, TestMessage1& mesg) {
        buffer.serialize(&mesg.a);
        buffer.serialize(&mesg.b);
        buffer.serialize(&mesg.c);
        buffer.serialize(&mesg.d);

        return true;
    }
};

// TEST_CASE("Test01", "[serializer]") {
//     TestMessage1 mesg = {
//         .a = 10,
//         .b = 100,
//         .c = 3,
//         .d = 0
//     };
//
//     tw::net::ByteBuffer buffer(1024);
//
//     buffer.write(mesg);
//
//     REQUIRE(mesg.a == 10);
//     REQUIRE(mesg.b == 100);
//     REQUIRE(mesg.c == 3);
//     REQUIRE(mesg.d == 0);
//
//     TestMessage1 mesg2 = {};
//
//     REQUIRE(mesg2.a == 0);
//     REQUIRE(mesg2.b == 0);
//     REQUIRE(mesg2.c == 0);
//     REQUIRE(mesg2.d == 0);
//
//     buffer.read(mesg2);
//
//     REQUIRE(mesg.a == mesg2.a);
//     REQUIRE(mesg.b == mesg2.b);
//     REQUIRE(mesg.c == mesg2.c);
//     REQUIRE(mesg.d == mesg2.d);
// }
//
// TEST_CASE("ByteBufferRemainingTest", "[serializer]") {
//     TestMessage1 mesg = {
//         .a = 10,
//         .b = 100,
//         .c = 3,
//         .d = 0
//     };
//
//     tw::net::ByteBuffer buffer(4 * sizeof(uint32_t) + 10);
//     buffer.write(mesg);
//
//     REQUIRE(buffer.remaining() == 10);
// }
//
// TEST_CASE("ByteBufferOverflowsTest1", "[serializer]") {
//     TestMessage1 mesg = {
//         .a = 10,
//         .b = 100,
//         .c = 3,
//         .d = 0
//     };
//
//     tw::net::ByteBuffer buffer(2);
//     buffer.write(mesg);
//
//     // nothing should be written
//     REQUIRE(buffer.remaining() == 2);
// }
//
// TEST_CASE("TestMultipleMessagesOnePacket", "[serializer]") {
//     TestMessage1 mesg = {
//         .a = 10,
//         .b = 100,
//         .c = 3,
//         .d = 0
//     };
//
//     tw::net::ByteBuffer buffer(1024);
//
//     buffer.write(mesg);
//     buffer.write(mesg);
//
//     REQUIRE(mesg.a == 10);
//     REQUIRE(mesg.b == 100);
//     REQUIRE(mesg.c == 3);
//     REQUIRE(mesg.d == 0);
//
//     TestMessage1 mesg2 = {};
//
//     REQUIRE(mesg2.a == 0);
//     REQUIRE(mesg2.b == 0);
//     REQUIRE(mesg2.c == 0);
//     REQUIRE(mesg2.d == 0);
//
//     buffer.read(mesg2);
//     REQUIRE(buffer.remaining() == sizeof(TestMessage1));
//     TestMessage1 mesg3 = {};
//     buffer.read(mesg3);
//
//     REQUIRE(mesg.a == mesg2.a); REQUIRE(mesg.a == mesg3.a);
//     REQUIRE(mesg.b == mesg2.b); REQUIRE(mesg.b == mesg3.b);
//     REQUIRE(mesg.c == mesg2.c); REQUIRE(mesg.c == mesg3.c);
//     REQUIRE(mesg.d == mesg2.d); REQUIRE(mesg.d == mesg3.d);
// }
//
// TEST_CASE("SerializationTest", "[serialization]") {
//     TestMessage1 mesg = {
//         .a = 10,
//         .b = 100,
//         .c = 3,
//         .d = 0
//     };
//
//     tw::net::ByteBuffer buffer(1024);
//     buffer.write(mesg);
//     char *ptr = (char*)&mesg.a;
// }
//
// #define MAX_MESG_SIZE 256
//
// struct TestMessageString {
//     uint32_t mesg_size;
//     char mesg[MAX_MESG_SIZE];
// };
//
// template<>
// class tw::net::Serializer<TestMessageString> {
// public:
//     static bool serialize(ByteBuffer& buffer, TestMessageString& mesg) {
//         buffer.serialize(&mesg.mesg_size);
//         buffer.bytes(mesg.mesg, mesg.mesg_size);
//         return true;
//     }
// };
//
// TEST_CASE("StringTest", "[serializer]") {
//     TestMessageString mesg = {
//         .mesg_size = sizeof("Hello World") - 1,
//         .mesg = "Hello World"
//     };
//
//     tw::net::ByteBuffer buffer(1024);
//
//     buffer.write(mesg);
//
//     TestMessageString mesg2 = {};
//
//     buffer.read(mesg2);
//     mesg2.mesg[mesg2.mesg_size] = 0;
//
//     REQUIRE(!strcmp(mesg2.mesg, "Hello World"));
// }
//
//
// #define MAX_ELEMENTS 10
//
// struct TestScalableMessageElement {
//     int32_t a, b, c, d;
// };
//
//
// template<>
// class tw::net::Serializer<TestScalableMessageElement> {
// public:
//     static bool serialize(ByteBuffer& buffer, TestScalableMessageElement& mesg) {
//         if(buffer.remaining() < 4 * sizeof(uint32_t)) {
//             return false;
//         }
//
//         buffer.serialize(&mesg.a);
//         buffer.serialize(&mesg.b);
//         buffer.serialize(&mesg.c);
//         buffer.serialize(&mesg.d);
//
//         return true;
//     }
// };
//
// struct TestScalableMessage {
//     uint32_t num_elements;
//     TestScalableMessageElement elements[MAX_ELEMENTS];
// };
//
// template<>
// class tw::net::Serializer<TestScalableMessage> {
// public:
//     static bool serialize(ByteBuffer& buffer, TestScalableMessage& mesg) {
//         buffer.serialize(&mesg.num_elements);
//         int i = 0;
//         for(i = 0; i < mesg.num_elements; i++) {
//             bool is_there = true;
//             buffer.serialize(&is_there);
//
//             if(!buffer.serialize(mesg.elements[i])) {
//                 break;
//             }
//         }
//         
//         return true;
//     }
// };
//
// TEST_CASE("ByteBufferOverflowsTest2", "[serializer]") {
//     TestScalableMessage mesg = { };
//
//     for(int i = 0; i < 10; i++) {
//         mesg.elements[i] = {
//             .a = 1 * i,
//             .b = 2,
//             .c = 3 * i,
//             .d = 4
//         };
//         mesg.num_elements++;
//     }
//
//     tw::net::ByteBuffer buffer(20);
//
//     buffer.write(mesg);
// }
//
//
// #include <glm/glm.hpp>
//
//
// template<>
// class tw::net::Serializer<glm::vec3> final {
// public:
//     static bool serialize(ByteBuffer& buffer, glm::vec3& value) {
//         buffer.serialize(&value.x);
//         buffer.serialize(&value.y);
//         buffer.serialize(&value.z);
//         return true;
//     }
// };
//
// TEST_CASE("Vec3Serialize", "[socket]") {
//     tw::net::ByteBuffer buffer(20);
//
//     glm::vec3 vec = glm::vec3(10.0f, 5.0f, 3.14f);
//     buffer.write(vec);
//
//     glm::vec3 vec2 = {};
//     buffer.read(vec2);
//
//     REQUIRE(vec == vec2);
// }
