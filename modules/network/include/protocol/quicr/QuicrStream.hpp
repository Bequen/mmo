
#include "NetworkError.hpp"
#include "protocol/quicr/QuicrConnection.hpp"
#include "tl/expected.hpp"

namespace tw::net::quicr {

class QuicrStream : Write<std::byte>, Read<std::byte> {
    QuicrConnection* m_connection;
    bool m_is_reliable;

public:
    QuicrStream(QuicrConnection* connection, bool is_reliable);

    tl::expected<size_t, NetworkError> write(std::span<std::byte> data) override {
        auto send_r = m_connection->send_message(data, m_is_reliable);
        if(!send_r) {
            return tl::make_unexpected(NetworkError::from_errno(CONNECTION_RESET));
        }

        return *send_r;
    }

    tl::expected<size_t, NetworkError> read_into(std::span<std::byte> target) override {
        auto read_r = m_connection->read_into(target);
        if(!read_r) {
            return tl::make_unexpected(NetworkError::from_errno(CONNECTION_RESET));
        }

        return *read_r;
    }
};

}
