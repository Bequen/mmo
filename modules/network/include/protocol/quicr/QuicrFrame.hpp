#pragma once

#include "protocol/quicr/QuicrFrameType.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace tw::net::quicr {

struct QuicrFrame {
public:
    uint64_t frame_number;
    FrameType type;
    bool is_reliable;
    std::vector<std::byte> content;

    static QuicrFrame make_hello() {
        QuicrFrame frame;

        frame.type = FrameType::Hello;
        frame.is_reliable = true;

        return frame;
    }

    static QuicrFrame make_hello_fin() {
        QuicrFrame frame;

        frame.type = FrameType::HelloFin;
        frame.is_reliable = true;

        return frame;
    }

    static QuicrFrame make_stream(std::vector<std::byte> content) {
        QuicrFrame frame;

        frame.type = FrameType::StreamBase;
        frame.is_reliable = false;
        frame.content = std::move(content);

        return frame;
    }

    static QuicrFrame make_ack(std::vector<std::uint32_t> content) {
        QuicrFrame frame;

        frame.type = FrameType::Ack;
        frame.is_reliable = true;
        frame.content = std::move(std::vector<std::byte>(
            std::as_bytes(std::span(content)).begin(),
            std::as_bytes(std::span(content)).end())
        );

        return frame;
    }
};

}
