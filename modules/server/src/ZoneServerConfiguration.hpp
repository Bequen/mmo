#pragma once

#include <optional>
#include <string>

struct PostgreSQLConnectionInfo {
    std::string host;
    int         port     = 5432;
    std::string dbname;
    std::string username;
    std::string password;

    std::string to_string() const {
        return "host=" + host
             + " port=" + std::to_string(port)
             + " dbname=" + dbname
             + " user=" + username
             + " password=" + password;
    }
};

struct TimescaleDbConfiguration {
    PostgreSQLConnectionInfo connection;
    std::string              metrics_table = "zone_metrics";
};

struct ZoneServerConfiguration {
    int quicr_port   = 8101;
    int cluster_port = 8102;
    std::optional<TimescaleDbConfiguration> timescaledb;
};
