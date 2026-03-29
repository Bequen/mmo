#include "TimescaleDbBandwidthMonitor.hpp"
#include <common/TracyQueue.hpp>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

TimescaleDbBandwidthMonitor::TimescaleDbBandwidthMonitor(const PostgreSqlConnectionInfo& connection_info) :
m_outbound_bucket(0),
m_inbound_bucket(0),
m_last_flush_time(std::chrono::steady_clock::now()) {
    try {
        m_connection = pqxx::connection(connection_info.to_string());
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to TimescaleDB: {}", e.what());
    }
}

void TimescaleDbBandwidthMonitor::flush() {
    ZoneScopedN("TimescaleDB Bandwidth Flush");
    pqxx::work tx{m_connection.value()};

    tx.exec("INSERT INTO messages VALUES (NOW(), $1, FALSE)", pqxx::params(m_outbound_bucket))
        .no_rows();
    tx.exec("INSERT INTO messages VALUES (NOW(), $1, TRUE)", pqxx::params(m_inbound_bucket))
        .no_rows();
    tx.commit();

    TracyPlot("outbound", (int64_t)m_outbound_bucket);
    TracyPlot("inbound", (int64_t)m_inbound_bucket);

    m_outbound_bucket = 0;
    m_inbound_bucket = 0;

    m_last_flush_time = std::chrono::steady_clock::now();
}

void TimescaleDbBandwidthMonitor::add_outbound(size_t size) {
    m_outbound_bucket += size;
    if(is_time_to_flush()) {
        flush();
    }
}

void TimescaleDbBandwidthMonitor::add_inbound(size_t size) {
    m_inbound_bucket += size;
    if(is_time_to_flush()) {
        flush();
    }
}
