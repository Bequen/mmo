#pragma once

#include "BandwidthMonitor.hpp"
#include <string>
#include <pqxx/pqxx>

struct PostgreSqlConnectionInfo {
    std::string host;
    int port;
    std::string dbname;
    std::string username;
    std::string password;

    const std::string to_string() const {
        return "host=" + host + " port=" + std::to_string(port) + " dbname=" + dbname + " user=" + username + " password=" + password;
    }
};

/**
 * Bandwidth data publisher to the TimescaleDB.
 */
class TimescaleDbBandwidthMonitor : public BandwidthMonitor {
    std::optional<pqxx::connection> m_connection;

    size_t m_inbound_bucket;
    size_t m_outbound_bucket;
    std::chrono::steady_clock::time_point m_last_flush_time;

    /**
     * Returns if we should flush;
     */
    bool is_time_to_flush() {
        return m_last_flush_time < std::chrono::steady_clock::now() - std::chrono::seconds(1);
    }

    void flush();

public:
    TimescaleDbBandwidthMonitor(const PostgreSqlConnectionInfo& connection_info);

    ~TimescaleDbBandwidthMonitor() {
    }

    void add_outbound(size_t size) override;

    void add_inbound(size_t size) override;
};
