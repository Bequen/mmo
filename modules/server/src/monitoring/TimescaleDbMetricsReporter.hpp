#pragma once

#include "MetricsReporter.hpp"
#include "ZoneServerConfiguration.hpp"
#include <chrono>
#include <optional>
#include <pqxx/pqxx>

class TimescaleDbMetricsReporter : public NetworkMetricsReporter {
    std::optional<pqxx::connection>        m_connection;
    std::string                            m_metrics_table;

    size_t m_outbound_bucket = 0;
    size_t m_inbound_bucket  = 0;
    size_t m_player_count    = 0;
    std::chrono::steady_clock::time_point  m_last_flush_time;

    bool is_time_to_flush() const {
        return m_last_flush_time < std::chrono::steady_clock::now() - std::chrono::seconds(1);
    }

    void flush();

public:
    explicit TimescaleDbMetricsReporter(const TimescaleDbConfiguration& config);

    void tick() override;
    void add_outbound(size_t size) override { m_outbound_bucket += size; }
    void add_inbound(size_t size)  override { m_inbound_bucket  += size; }
    void set_player_count(size_t count) override { m_player_count = count; }
};
