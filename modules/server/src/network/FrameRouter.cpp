#include "FrameRouter.hpp"
#include "network/InboundMessage.hpp"

#include <spdlog/spdlog.h>

namespace tw::net {

FrameRouter::FrameRouter(MessageQueue<InboundMessage>* inbound_queue) :
    m_inbound_queue(inbound_queue) {}

void FrameRouter::route(uint32_t session_id, const std::vector<Frame>& frames) {
    for(auto& frame : frames) {
        spdlog::info("Rounting frame");

        if(frame.frame_type() == quicr::FrameType::StreamBase) {
            m_inbound_queue->push(InboundMessage(session_id, frame.buffer()));
            continue;
        } else {
            spdlog::warn("Unknown frame type: {}", (uint32_t)frame.frame_type());
        }
    }
}

}
