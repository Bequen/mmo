#pragma once

#include <vector>

namespace tw::net {

struct OutboundMessage {
  std::vector<std::byte> payload;

  OutboundMessage(std::vector<std::byte> payload)
      : payload(std::move(payload)) {}
};

} // namespace tw::net
