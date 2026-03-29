#include "messenger/MessengerDebugLog.hpp"
#include <nlohmann/json.hpp>

MessengerDebugLog::MessengerDebugLog(const std::string& log_file_path) : m_log_file(log_file_path) {
    if (!m_log_file.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }
}

MessengerDebugLog::~MessengerDebugLog() {
    m_log_file.close();
}
