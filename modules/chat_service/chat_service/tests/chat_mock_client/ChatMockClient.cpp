#include "Address.hpp"
#include "MessageSession.hpp"
#include "MessageRegistry.hpp"
#include "Chat.pb.h"

#include <spdlog/spdlog.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <print>
#include <string>
#include <sys/select.h>
#include <thread>
#include <unistd.h>
#include <vector>

static std::atomic<bool> g_quit{false};

static void on_signal(int) { g_quit.store(true); }

static void register_signal_handler() {
    struct sigaction sa{};
    sa.sa_handler = on_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

template<typename T>
static std::vector<std::byte> serialize(const T& msg) {
    std::vector<std::byte> buf(msg.ByteSizeLong());
    (void)msg.SerializeToArray(buf.data(), static_cast<int>(buf.size()));
    return buf;
}

int main(int argc, char* argv[]) {
    register_signal_handler();

    std::string host        = "127.0.0.1";
    int         port        = 8099;
    uint64_t    channel_id  = 0;
    bool        channel_set = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if      (arg == "--host"    && i + 1 < argc) host = argv[++i];
        else if (arg == "--port"    && i + 1 < argc) port = std::stoi(argv[++i]);
        else if (arg == "--channel" && i + 1 < argc) {
            channel_id  = std::stoull(argv[++i]);
            channel_set = true;
        }
    }

    if (!channel_set) {
        std::println(stderr, "Usage: {} [--host <ip>] [--port <port>] --channel <id>", argv[0]);
        return 1;
    }

    tw::MessageSession session(tw::net::Address{std::string{host}, port});

    spdlog::info("Connecting to {}:{}...", host, port);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!session.is_established()) {
        if (std::chrono::steady_clock::now() > deadline) {
            spdlog::error("Connection timed out");
            return 1;
        }
        session.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    spdlog::info("Connected. Joining channel {}...", channel_id);

    session.set_handler(tw::Message<mmo::chat::ChatMessageBroadcastRequest>::value,
        [](std::span<const std::byte> data) {
            mmo::chat::ChatMessageBroadcastRequest bcast;
            bcast.ParseFromArray(data.data(), static_cast<int>(data.size()));
            std::println("[ch:{}] <{}> {}", bcast.channel_id(), bcast.sender_id(), bcast.message());
        });

    mmo::chat::JoinChannelRequest join;
    join.set_channel_id(channel_id);
    (void)session.request(
        tw::Message<mmo::chat::JoinChannelRequest>::value,
        serialize(join),
        [channel_id](std::span<const std::byte> data) {
            mmo::chat::JoinChannelResponse resp;
            resp.ParseFromArray(data.data(), static_cast<int>(data.size()));
            std::println("Joined channel {} successfully.", channel_id);
        });

    std::println("Joined channel {}. Type a message and press Enter. Ctrl+C to quit.", channel_id);

    while (!g_quit.load()) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv{0, 10'000};
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
            std::string line;
            if (!std::getline(std::cin, line)) break;
            if (!line.empty()) {
                mmo::chat::SendChatMessageRequest msg;
                msg.set_channel_id(channel_id);
                msg.set_message(line);
                (void)session.request(
                    tw::Message<mmo::chat::SendChatMessageRequest>::value,
                    serialize(msg),
                    [channel_id](std::span<const std::byte> data) {
                        mmo::chat::SendChatMessageResponse resp;
                        resp.ParseFromArray(data.data(), static_cast<int>(data.size()));
                        std::println("Message sent to channel {}.", channel_id);
                    });
            }
        }

        session.update();
    }

    return 0;
}
