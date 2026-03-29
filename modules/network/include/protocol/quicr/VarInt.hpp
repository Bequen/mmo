#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace tw::net::quicr {

struct VarInt {

    uint64_t value = 0;
    // byte length of the number in the stream. Use to adjust offset.
    size_t   bytes = 0;

    VarInt(uint64_t value) {
        if (value <= 63) {
            bytes = 1;
        } else if (value <= 16383) {
            bytes = 2;
        } else if (value <= 1073741823) {
            bytes = 4;
        }

        bytes = 8;

        this->value = value;
    }

    VarInt(uint64_t value, size_t bytes) {
        this->value = value;
        this->bytes = bytes;
    }

    static std::optional<VarInt> decode(std::span<const std::byte> in) {
        if (in.empty()) return std::nullopt;
        return VarInt(*(uint64_t*)in.data(), sizeof(uint64_t));

        // uint8_t b0     = std::to_integer<uint8_t>(in[0]);
        // uint8_t prefix = (b0 >> 6) & 0x03;

        // size_t len = size_t(1) << prefix; // 1, 2, 4, 8

        // if (in.size() < len) return std::nullopt;

        // uint64_t v = (uint64_t)(b0 & 0x3f);
        // for (size_t i = 1; i < len; ++i) {
        //     v = (v << 8) | std::to_integer<uint8_t>(in[i]);
        // }

        // return VarInt(v, len);
    }

    size_t encode(std::vector<std::byte>& out) {
        // if (value <= 63) {
        //     out.push_back(std::byte(value));
        //     return 1;
        // }
        // if (value <= 16383) {
        //     out.push_back(std::byte(0x40 | ((value >> 8) & 0x3f)));
        //     out.push_back(std::byte(value & 0xff));
        //     return 2;
        // }
        // if (value <= 1073741823) {
        //     out.push_back(std::byte(0x80 | ((value >> 24) & 0x3f)));
        //     out.push_back(std::byte((value >> 16) & 0xff));
        //     out.push_back(std::byte((value >> 8)  & 0xff));
        //     out.push_back(std::byte(value & 0xff));
        //     return 4;
        // }
        // 8-byte
        // out.push_back(std::byte(0xc0 | ((value >> 56) & 0x3f)));
        // out.push_back(std::byte((value >> 48) & 0xff));
        // out.push_back(std::byte((value >> 40) & 0xff));
        // out.push_back(std::byte((value >> 32) & 0xff));
        // out.push_back(std::byte((value >> 24) & 0xff));
        // out.push_back(std::byte((value >> 16) & 0xff));
        // out.push_back(std::byte((value >> 8)  & 0xff));
        // out.push_back(std::byte(value & 0xff));

        // insert value into span
        for (int i = 0; i < 8; ++i) {
            out.push_back(std::byte((value >> (i * 8)) & 0xFF));
        }

        return 8;
    }
};

}
