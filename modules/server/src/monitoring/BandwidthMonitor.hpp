#pragma once

#include <cstddef>

class BandwidthMonitor {
public:
    virtual void add_outbound(size_t size) = 0;

    virtual void add_inbound(size_t size) = 0;
};
