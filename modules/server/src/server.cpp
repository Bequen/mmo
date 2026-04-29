#include "ZoneServer.hpp"
#include "ZoneServerConfiguration.hpp"

#include <getopt.h>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <print>

static constexpr int kDefaultQuicrPort   = 8101;
static constexpr int kDefaultClusterPort = 8102;

static void print_usage(const char* argv0) {
    std::println("Usage: {} [--quicr-port <port>] [--cluster-port <port>]", argv0);
    std::println("  --quicr-port    UDP port for player connections  (default: {})", kDefaultQuicrPort);
    std::println("  --cluster-port  UDP port for zone-server peering (default: {})", kDefaultClusterPort);
    std::println("");
    std::println("TimescaleDB (all optional, enabled when TIMESCALEDB_HOST is set):");
    std::println("  TIMESCALEDB_HOST      host name");
    std::println("  TIMESCALEDB_PORT      port         (default: 5432)");
    std::println("  TIMESCALEDB_DB        database     (default: mmo)");
    std::println("  TIMESCALEDB_USER      username     (default: mmo)");
    std::println("  TIMESCALEDB_PASSWORD  password");
    std::println("  TIMESCALEDB_TABLE     metrics table (default: zone_metrics)");
}

static auto env(const char* name, const char* fallback = "") -> std::string {
    const char* v = std::getenv(name);
    return v ? v : fallback;
}

static std::optional<TimescaleDbConfiguration> timescaledb_from_env() {
    const std::string host = env("TIMESCALEDB_HOST");
    if (host.empty()) return std::nullopt;

    const std::string port_str = env("TIMESCALEDB_PORT", "5432");
    return TimescaleDbConfiguration{
        .connection = {
            .host     = host,
            .port     = std::atoi(port_str.c_str()),
            .dbname   = env("TIMESCALEDB_DB",       "mmo"),
            .username = env("TIMESCALEDB_USER",     "mmo"),
            .password = env("TIMESCALEDB_PASSWORD"),
        },
        .metrics_table = env("TIMESCALEDB_TABLE", "zone_metrics"),
    };
}

int main(int argc, char* argv[]) {
    ZoneServerConfiguration config;

    static const option long_opts[] = {
        { "quicr-port",   required_argument, nullptr, 'q' },
        { "cluster-port", required_argument, nullptr, 'c' },
        { "help",         no_argument,       nullptr, 'h' },
        { nullptr,        0,                 nullptr,  0  },
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "q:c:h", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'q': config.quicr_port   = std::atoi(optarg); break;
            case 'c': config.cluster_port = std::atoi(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default:  print_usage(argv[0]); return 1;
        }
    }

    config.timescaledb = timescaledb_from_env();

    spdlog::info("quicr-port={} cluster-port={} timescaledb={}",
        config.quicr_port, config.cluster_port,
        config.timescaledb ? config.timescaledb->connection.host : "disabled");

    try {
        tw::net::ZoneServer server(std::move(config));
        server.run();
    } catch (const std::exception& e) {
        std::println("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
