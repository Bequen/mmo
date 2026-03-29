#pragma once

#include <chrono>
#include <glm/glm.hpp>

#include "TcpStream.hpp"
#include "messenger/Messenger.hpp"
#include "common.hpp"

namespace tw::net {

typedef uint32_t PlayerClientId;

class PlayerClient {
private:
    const uint32_t MAX_CONSECUTIVE_FAILURES = 10;
    const std::chrono::milliseconds HEARTBEAT_INTERVAL = std::chrono::milliseconds(5000);

    PlayerClientId m_id;

    Messenger<std::byte, TcpStream> m_messenger;

    std::string m_name;
    uint32_t m_entity_id;

    uint32_t m_last_frame_idx;
    bool m_frame_responded = true;

    uint32_t m_first_time;
    uint32_t m_num_times;

    uint32_t m_consecutive_failures;
    std::chrono::steady_clock::time_point m_last_heartbeat;

public:
    GET(m_last_frame_idx, last_frame_idx);

    void add_failure() {
        m_consecutive_failures++;
        spdlog::error("PlayerClient failure {}", m_consecutive_failures);
    }

    bool is_disconnected() const {
        return m_consecutive_failures >= MAX_CONSECUTIVE_FAILURES ||
                m_last_heartbeat + HEARTBEAT_INTERVAL < std::chrono::steady_clock::now();
    }

    PlayerClient(const PlayerClient&) = delete;
    PlayerClient(PlayerClient&&) = default;

    PlayerClient& operator=(const PlayerClient&) = delete;
    PlayerClient& operator=(PlayerClient&&) = delete;

    GET(m_id, id);
    GET_MUT_REF(m_messenger, messenger);

    PlayerClient(PlayerClientId id, std::string name,
        Messenger<std::byte, TcpStream>&& messenger
    ) :
        m_id(id),
        m_entity_id(id),
        m_messenger{std::move(messenger)},
        m_name(name),
        m_last_frame_idx(0),
        m_consecutive_failures(0),
        m_last_heartbeat(std::chrono::steady_clock::now())
    { }

    void set_received() {
        m_last_heartbeat = std::chrono::steady_clock::now();
        m_consecutive_failures = 0;
    }

    void set_last_frame_idx(uint32_t frame_idx) {
        if(frame_idx > m_last_frame_idx) {
            m_last_frame_idx = frame_idx;
            m_frame_responded = false;
        }
    }
};

}
