#pragma once

#include "MetricsReporter.hpp"

class NullMetricsReporter : public NetworkMetricsReporter {
public:
    void tick() override {}
    void add_outbound(size_t) override {}
    void add_inbound(size_t) override {}
    void set_player_count(size_t) override {}
};
