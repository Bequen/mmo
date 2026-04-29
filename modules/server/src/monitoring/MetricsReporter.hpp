#pragma once

#include <cstddef>

class NetworkMetricsReporter {
public:
    virtual ~NetworkMetricsReporter() = default;

    virtual void tick() = 0;
    virtual void add_outbound(size_t size) = 0;
    virtual void add_inbound(size_t size) = 0;
    virtual void set_player_count(size_t player_count) = 0;
};
