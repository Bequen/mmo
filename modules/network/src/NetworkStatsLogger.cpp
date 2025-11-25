#include "StatsLogger.hpp"

namespace tw::net {

NetworkStatsLogger* NetworkStatsLogger::_instance;

NetworkStatsLogger* NetworkStatsLogger::instance() {
    if(_instance == nullptr) {
        _instance = new NetworkStatsLogger();
    }

    return _instance;
}

}

