#pragma once

#include "QuicrFrame.hpp"
#include "protocol/quicr/QuicrPacketType.hpp"

#include <optional>

namespace tw::net::quicr {

class QuicrPacket {
public:
  QuicrPacketType type;

  uint64_t destination_id;
  uint64_t local_id;

  bool require_ack;
  std::optional<uint32_t> packet_number;

  uint32_t length;

  std::vector<QuicrFrame> frames;

  QuicrPacket()
      : type(QuicrPacketType::Unknown), destination_id(0), local_id(0),
        require_ack(false), packet_number({}), length(0), frames() {}
};

} // namespace tw::net::quicr
