#include "ChatServerController.hpp"

#include <spdlog/spdlog.h>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>

static std::atomic<bool> g_quit{false};

static void on_signal(int) {
    g_quit.store(true);
}

static void register_signal_handler() {
    struct sigaction sa{};
    sa.sa_handler = on_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

int main() {
    register_signal_handler();

    spdlog::info("Starting chat server on port {}", tw::chat::CHAT_DEFAULT_PORT);

    tw::chat::ChatServerController controller;

    while (!g_quit.load()) {
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    spdlog::info("Chat server shutting down");
    return 0;
}
