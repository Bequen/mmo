#pragma once

#include "packets/Packet.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <fstream>

class MessengerDebugLog {
public:
    MessengerDebugLog(MessengerDebugLog&& m) :
        m_log_file(std::move(m.m_log_file))
    { }

    MessengerDebugLog(const std::string& log_file_path);
    ~MessengerDebugLog();

    template<typename T>
    void log_send(const T& message) {
        spdlog::info("Sending [{}]: {}", (int)Message<T>::value, message.DebugString());
    }

    template<typename T>
    void log_recv(const T& message) {
        spdlog::info("Received [{}]: {}", (int)Message<T>::value, message.DebugString());
    }

private:
    std::ofstream m_log_file;
};
