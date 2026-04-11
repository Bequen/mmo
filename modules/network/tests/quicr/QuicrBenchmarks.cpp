#include "TcpListener.hpp"
#include "bytebuffer/ByteBufferReader.hpp"
#include "bytebuffer/ByteBufferWriter.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"
#include "io/Read.hpp"
#include "io/Write.hpp"

#include <iostream>
#include <fstream>
#include <chrono>
#include <ratio>

#include <tracy/Tracy.hpp>

#define PORT 6970

#define FRAMES_PER_SECOND 60

#define SECONDS_OF_TESTING 10

void server_func(std::atomic<bool>& is_done, tw::net::Write<std::byte>* writer, tw::net::Read<std::byte>* reader) {
    double value = 0.0f;

    std::vector<std::byte> inbound_buffer(1200);
    size_t inbound_length = 0;

    std::vector<std::byte> outbound_buffer(1200);

    while(!is_done) {
        auto read_r = reader->read_into(std::span(inbound_buffer).subspan(inbound_length));
        inbound_length += *read_r;

        uint32_t frame_number = 0;

        auto decoder = tw::net::ByteBufferReader(std::span(inbound_buffer).subspan(0, inbound_length));
        while(decoder.remaining()) {
            auto read_r = decoder.pop_bytes(&frame_number);
            if(!read_r) {
                break;
            }

            double velocity = 0.0f;
            read_r = decoder.pop_bytes(&velocity);
            if(!read_r) {
                break;
            }

            value += velocity;

            // encode response
            tw::net::ByteBufferWriter encoder((std::span<std::byte>(outbound_buffer)));
            encoder.write_bytes(&frame_number);

            encoder.write_bytes(&value);

            auto write_r = writer->write(std::span(outbound_buffer).subspan(0, encoder.length()));
            if(!write_r) {
                break;
            }
        }

        // move bytes back
        memcpy(inbound_buffer.data(), inbound_buffer.data() + decoder.position(), decoder.remaining());
    }
}

void client_func(std::atomic<bool>& is_done, tw::net::Write<std::byte>* writer, tw::net::Read<std::byte>* reader) {
    std::vector<std::byte> outbound_buffer(1200);
    std::vector<std::byte> inbound_buffer(1200);

    uint32_t frame_number = 0;

    while(!is_done) {
        tw::net::ByteBufferWriter writer(outbound_buffer);

        writer.write_bytes(&frame_number);
        double random = std::sin(frame_number);
        writer.write_bytes(&random);

        // writer.write_bytes();
    }
}

double derivation_func(uint32_t frame_number) {
    return std::sin((double)frame_number / 25.0f);
}

void test_quic() {
    std::atomic<bool> client_is_done = false;

    std::thread server_thread([&]() {
        auto server_endpoint = tw::net::quicr::QuicrEndpoint::create().value();
        assert(server_endpoint.bind(PORT));

        auto listener_r = tw::net::quicr::QuicrConnectionListener::listen(&server_endpoint);
        auto listener = std::move(*listener_r);
        server_endpoint.assign_listener(&listener);

        tw::net::quicr::QuicrConnection* connection = nullptr;
        while(connection == nullptr) {
            server_endpoint.poll();
            connection = listener.listen();
        }

        uint32_t frame_number = 0;

        std::vector<std::byte> buffer(1200);
        std::vector<std::byte> outbound_buffer(1200);

        double value = 0.0f;

        while(true) {
            if(client_is_done) {
                break;
            }

            server_endpoint.poll();

            auto read_r = connection->read_into(buffer);

            if(read_r.has_value() && *read_r > 0) {
                ZoneScopedN("Server read");
                tw::net::ByteBufferReader reader((std::span<std::byte>(buffer).subspan(0, read_r.value())));

                uint32_t frame_number = 0;
                reader.pop_bytes(&frame_number);

                double velocity = 0;
                reader.pop_bytes(&velocity);

                value += velocity;
            }

            tw::net::ByteBufferWriter writer(outbound_buffer);
            writer.write_bytes(&frame_number);
            writer.write_bytes(&value);

            auto send_r = connection->send_message(std::span(outbound_buffer).subspan(0, writer.length()), false);
            assert(send_r.has_value());

            server_endpoint.poll();
            frame_number++;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });

    std::thread client_thread([&client_is_done]() {
        auto client_endpoint = tw::net::quicr::QuicrEndpoint::create().value();
        auto connection = client_endpoint.connect({"127.0.0.1", PORT}).value();

        while(connection->state() != tw::net::quicr::Established) {
            client_endpoint.poll();
        }

        std::vector<std::byte> outbound_buffer(1200);
        std::vector<std::byte> inbound_buffer(1200);

        std::map<uint32_t, std::chrono::steady_clock::time_point> sent_at;

        int32_t countdown = FRAMES_PER_SECOND * SECONDS_OF_TESTING;

        std::ofstream quicr_csv("quicr.csv");
        std::ofstream quicr_integration_csv("quicr_integration.csv");
        uint32_t frame_number = 0;
        double position = 0;

        while(true) {
            if(countdown <= 0) {
                client_is_done.store(true);
                break;
            }

            client_endpoint.poll();
            tw::net::ByteBufferWriter writer(outbound_buffer);

            writer.write_bytes(&frame_number);
            double random = derivation_func(frame_number);
            writer.write_bytes(&random);

            auto send_r = connection->send_message(std::span(outbound_buffer).subspan(0, writer.length()), false);
            assert(send_r.has_value());

            sent_at.emplace(frame_number, std::chrono::steady_clock::now());

            auto read_r = connection->read_into(std::span<std::byte>(inbound_buffer));
            if(read_r.has_value() && *read_r > 0) {
                tw::net::ByteBufferReader reader(std::span<std::byte>(inbound_buffer).subspan(0, read_r.value()));

                uint32_t _frame_number = 0;
                reader.pop_bytes(&_frame_number);

                if(!sent_at.contains(_frame_number)) {
                    spdlog::warn("Frame {} not sent", _frame_number);
                    continue;
                }

                reader.pop_bytes(&position);

                auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - sent_at[_frame_number]).count();

                spdlog::info("Frame {} received after {}ms", _frame_number, rtt);
                sent_at.erase(_frame_number);
                quicr_csv << _frame_number << "," << rtt << "," << position << std::endl;
                countdown--;
            }

            quicr_integration_csv << frame_number <<  "," << position << std::endl;

            client_endpoint.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            frame_number++;
        }
    });

    server_thread.join();
    client_thread.join();
}

void test_tcp() {
    std::atomic<bool> client_is_done(false);

    std::thread server_thread([&]() {
        tw::net::Address address {"127.0.0.1", PORT};
        auto server_listener = tw::net::TcpListener::listen(address, PORT).value();

        std::optional<tw::net::TcpStream> stream;
        while(true) {
            auto stream_r = server_listener.listen();
            if(stream_r) {
                stream = std::move(*stream_r);
                break;
            }
        }
        auto non_blocking_r = stream->set_non_blocking();

        std::vector<std::byte> buffer(1200);
        std::vector<std::byte> outbound_buffer(1200);
        uint32_t frame_number = 0;
        int32_t countdown = FRAMES_PER_SECOND * SECONDS_OF_TESTING;

        double value = 0.0f;

        while(!client_is_done) {
            auto read_r = stream->read_into(buffer);

            if(read_r.has_value() && *read_r > 0) {
                tw::net::ByteBufferReader reader((std::span<std::byte>(buffer).subspan(0, read_r.value())));

                while(reader.remaining() > 0) {
                    uint32_t _frame_number = 0;
                    reader.pop_bytes(&_frame_number);

                    double velocity = 0;
                    reader.pop_bytes(&velocity);
                    value += velocity;

                    countdown--;
                }
            }

            tw::net::ByteBufferWriter writer(outbound_buffer);
            writer.write_bytes(&frame_number);
            writer.write_bytes(&value);

            auto send_r = stream->write(std::span(outbound_buffer).subspan(0, writer.length()));
            assert(send_r.has_value());

            frame_number++;

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });

    std::thread client_thread([&client_is_done]() {
        auto client_stream = tw::net::TcpStream::connect({"127.0.0.1", PORT}).value();
        auto non_blocking_r = client_stream.set_non_blocking();

        std::vector<std::byte> outbound_buffer(1200);
        std::vector<std::byte> inbound_buffer(1200);

        std::map<uint32_t, std::chrono::steady_clock::time_point> sent_at;
        int32_t countdown = FRAMES_PER_SECOND * SECONDS_OF_TESTING;
        uint32_t frame_number = 0;

        // open file tcp.csv
        std::ofstream tcp_csv("tcp.csv");
        std::ofstream tcp_integration_csv("tcp_integration.csv");

        double position = 0.0f;

        while(true) {
            if(countdown <= 0) {
                client_is_done.store(true);
                break;
            }

            tw::net::ByteBufferWriter writer(outbound_buffer);

            writer.write_bytes(&frame_number);
            double random = derivation_func(frame_number);
            writer.write_bytes(&random);

            auto send_r = client_stream.write(std::span(outbound_buffer).subspan(0, writer.length()));
            assert(send_r.has_value());

            sent_at.emplace(frame_number, std::chrono::steady_clock::now());
            frame_number++;

            auto read_r = client_stream.read_into(std::span<std::byte>(inbound_buffer));
            if(read_r.has_value() && *read_r > 0) {
                tw::net::ByteBufferReader reader(std::span<std::byte>(inbound_buffer).subspan(0, read_r.value()));

                while(reader.remaining() > 0) {
                    uint32_t _frame_number = 0;
                    reader.pop_bytes(&_frame_number);

                    reader.pop_bytes(&position);

                    auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - sent_at[_frame_number]).count();

                    spdlog::info("Frame {} received after {}ms", _frame_number, rtt);
                    tcp_csv << _frame_number << "," << rtt << "," << position << std::endl;
                    countdown--;
                }
            }

            tcp_integration_csv << frame_number << ","  << position << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        tcp_csv.close();
    });

    server_thread.join();
    client_thread.join();
}

int main() {
    test_quic();

    test_tcp();

    return 0;
}
