#include <cstdint>
#include <exception>
#include <stdexcept>
#include <print>

#include "Address.hpp"
#include "TcpSocket.hpp"
#include "entt/entt.hpp"
#include "exception/SocketClosedException.hpp"
#include "WorldServer.hpp"

#define PORT 8080
#define MAX_LINE 1024

// find first free socket -> sometimes socket remaing stuck in WAIT state, even
// after closing
int bind_to_free_port(tw::net::TcpSocket& socket, int from, int to) {
    for(int32_t i = from; i < to; i++) {
        try {
            socket.bind(tw::net::Address{{}, i});
            return i;
        } catch(std::runtime_error& err) { 
            std::println("Runtime error: {}", err.what());
        }
        catch(tw::net::SocketClosedException err) {
            std::println("Socket closed error: {}", err.what());
        }
    }

    return -1;
}

int main() {
    tw::net::TcpSocket socket;
    int port = bind_to_free_port(socket, 8100, 9000);
    socket.set_non_blocking();

    std::println("Bound to port: {}", port);

    try {
        tw::net::WorldServer server(std::move(socket));
        server.run();
    } catch(std::exception e) {
        std::println("Exception: {}", e.what());
    }
    return 0;
}
