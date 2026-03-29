#pragma once

#include "frames/Frame.hpp"
#include "network/InboundMessage.hpp"
#include "network/MessageDeserializer.hpp"
#include "network/MessageQueue.hpp"

namespace tw::net {

class FrameRouter {
    MessageQueue<InboundMessage>* m_inbound_queue;

    MessageDeserializer m_deserializer;

public:
    FrameRouter(MessageQueue<InboundMessage>* inbound_queue);

    void route(uint32_t session_id, const std::vector<Frame>& frames);
};

}
