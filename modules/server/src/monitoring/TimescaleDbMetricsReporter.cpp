#include "TimescaleDbMetricsReporter.hpp"
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

TimescaleDbMetricsReporter::TimescaleDbMetricsReporter(const TimescaleDbConfiguration& config)
    : m_metrics_table(config.metrics_table),
      m_last_flush_time(std::chrono::steady_clock::now())
{
    try {
        m_connection = pqxx::connection(config.connection.to_string());
        spdlog::info("Connected to TimescaleDB at {}:{}", config.connection.host, config.connection.port);
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to TimescaleDB: {}", e.what());
    }
}

void TimescaleDbMetricsReporter::flush() {
    if (!m_connection) return;

    ZoneScopedN("TimescaleDB Metrics Flush");

    try {
        pqxx::work tx{*m_connection};

        const auto table = m_connection->quote_name(m_metrics_table);
        tx.exec("INSERT INTO " + table + " (time, outbound, inbound, player_count) VALUES (NOW(), $1, $2, $3)",
                pqxx::params(m_outbound_bucket, m_inbound_bucket, m_player_count))
            .no_rows();
        tx.commit();

        TracyPlot("outbound",     (int64_t)m_outbound_bucket);
        TracyPlot("inbound",      (int64_t)m_inbound_bucket);
        TracyPlot("player_count", (int64_t)m_player_count);
    } catch (const pqxx::broken_connection& e) {
        spdlog::error("TimescaleDB connection lost, disabling metrics: {}", e.what());
        m_connection.reset();
    } catch (const pqxx::sql_error& e) {
        spdlog::error("TimescaleDB insert failed (query: {}): {}", e.query(), e.what());
    } catch (const std::exception& e) {
        spdlog::error("TimescaleDB flush error: {}", e.what());
    }

    m_outbound_bucket = 0;
    m_inbound_bucket  = 0;
    m_last_flush_time = std::chrono::steady_clock::now();
}

void TimescaleDbMetricsReporter::tick() {
    if (is_time_to_flush()) flush();
}
