#pragma once

#include <cstdint>
#include <queue>

namespace tw::net {

template<typename T>
class MessageQueue {
    std::queue<T> m_queue;

public:
    MessageQueue() : m_queue() {

    }

    bool is_empty() {
        return m_queue.empty();
    }

    void push(T mesg) {
        m_queue.push(mesg);
    }

    T pop() {
        auto mesg = m_queue.front();
        m_queue.pop();
        return mesg;
    }
};

}
