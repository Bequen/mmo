#include "frames/FrameCodec.hpp"

#include "bytebuffer/ByteBufferDecoder.hpp"

namespace tw::net::frame {

// static void encode(ByteBuffer& target, const Frame& frame) {
//     if(frame.frame_type() == quicr::FrameType::StreamBase) {
//         uint32_t frame_type = frame.frame_type();
//         target.write_bytes(&frame_type, sizeof(frame_type));

//         uint32_t length = frame.buffer().size();
//         target.write_bytes(&length, sizeof(length));

//         target.write_bytes((void*)frame.buffer().data(), frame.buffer().size());
//         return;
//     }

//     spdlog::warn("Unknown frame type");
// }

// static std::vector<Frame> decode(ByteBuffer& bytes) {
//     spdlog::info("Parsing frames");
//     std::vector<Frame> frames;

//     // parse first byte as packet type
//     if(bytes.remaining_read() < 1) {
//         spdlog::warn("Nothing to read");
//         return {};
//     }

//     size_t offset = 0;
//     size_t magic_search_length = 0;

//     while (offset < bytes.remaining_read()) {
//         if(magic_search_length > 3) {
//             break;
//         }

//         auto magic = ByteBufferDecoder<uint32_t>::decode(bytes);
//         if(magic != 0xDEADBEEF) {
//             spdlog::warn("DEADBEEF not found");
//             bytes.skip(1);
//             magic_search_length++;
//             continue;
//         }

//         auto t = ByteBufferDecoder<uint32_t>::decode(bytes, 4);
//         if(!t) {
//             spdlog::warn("Failed to decode frame type bytes");
//             break;
//         }

//         if (*t == quicr::FrameType::Padding) {
//             spdlog::info("padding");
//             bytes.skip(2 * sizeof(uint32_t));
//             continue;
//         }

//         if (*t == quicr::FrameType::KeepAlive) {
//             spdlog::info("keep alive");
//             bytes.skip(2 * sizeof(uint32_t));
//             // TODO: dispatch to connection if you want per-conn ping
//             continue;
//         }

//         if (*t == quicr::FrameType::StreamBase) {
//             spdlog::info("Stream!");
//             auto length_r = ByteBufferDecoder<uint32_t>::decode(bytes, 8);
//             if(!length_r) {
//                 spdlog::warn("Length could not be decoded");
//                 continue;
//             }

//             std::vector<std::byte> payload(*length_r + 4 * sizeof(uint32_t));
//             auto popped_bytes = bytes.pop_bytes(payload); // skip length field
//             spdlog::info("Popped: {} ", popped_bytes);
//             if(popped_bytes == 0) {
//                 spdlog::warn("Not enought bytes in the buffer");
//                 break;
//             }

//             std::vector<std::byte> payload_without_header(payload.begin() + 3 * sizeof(uint32_t), payload.end());

//             frames.emplace_back(quicr::FrameType::StreamBase, payload_without_header);
//             continue;
//         }

//         bytes.skip(2 * sizeof(uint32_t));
//         spdlog::warn("unknown frame {}", *t);

//         // Unknown frame — can't skip safely
//         break;
//     }

//     spdlog::info("Parsed {} frames", frames.size());

//     return frames;
// }

}
