#include <cstdint>
#include <exception>
#include <spdlog/spdlog.h>
#include <print>

#include "Address.hpp"
#include "TcpListener.hpp"
#include "entt/entt.hpp"
#include "WorldServer.hpp"

#define PORT 8080
#define MAX_LINE 1024

// find first free socket -> sometimes socket remaing stuck in WAIT state, even after closing
std::optional<tw::net::TcpListener> bind_to_free_port(int from, int to) {
    for(int32_t i = from; i < to; i++) {
        auto listener = tw::net::TcpListener::listen(tw::net::Address({}, i), i);
        if(listener.has_value()) {
            return std::move(listener.value());
        }
    }

    return {};
}

int main() {
    // auto listener = bind_to_free_port(8100, 9000);
    // if(!listener.has_value()) {
    //     spdlog::error("No free port found for server");
    //     return 1;
    // }

    try {
        tw::net::WorldServer server(8100, 8101);
        server.run();
    } catch(std::exception e) {
        std::println("Exception: {}", e.what());
        throw e;
    }

    return 0;
}
