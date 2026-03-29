#pragma once

#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#include <string>
#include <cstring>
#include <optional>
#include <sys/socket.h>
#include <format>

namespace tw::net {

/**
 * IP Address
 */
struct Address {
private:
    sockaddr_storage m_storage {};

public:
    Address(const std::optional<std::string>& address, int port) {
        std::memset((char*)&this->m_storage, 0, sizeof(this->m_storage));

        auto& addr = reinterpret_cast<sockaddr_in&>(m_storage);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = address.has_value()
            ? inet_addr(address->c_str())
            : INADDR_ANY;
    }


    Address(sockaddr_storage& storage)
        : m_storage(storage)
    { }

    Address(sockaddr_storage&& storage)
        : m_storage(storage)
    { }

    /** Return a const pointer suitable for connect / sendto / bind. */
    const struct sockaddr* sockaddr() const {
        return reinterpret_cast<const struct sockaddr*>(&m_storage);
    }

    /** Return a mutable pointer suitable for recvfrom / accept. */
    struct sockaddr* sockaddr_mut() {
        return reinterpret_cast<struct sockaddr*>(&m_storage);
    }

    /** Return the size of the active address (depends on family). */
    socklen_t socklen() const {
        switch (m_storage.ss_family) {
            case AF_INET:  return sizeof(sockaddr_in);
            case AF_INET6: return sizeof(sockaddr_in6);
            default:       return sizeof(sockaddr_storage);
        }
    }

    /** Mutable reference to the raw storage — useful when you need to pass
     *  a sockaddr_storage* to recvfrom together with a socklen_t. */
    sockaddr_storage& storage() { return m_storage; }
    const sockaddr_storage& storage() const { return m_storage; }

    sa_family_t family() const { return m_storage.ss_family; }

    /** Returns the raw network-order IPv4 address, or 0 if not AF_INET. */
    uint32_t ipv4_addr_raw() const {
        if (m_storage.ss_family == AF_INET) {
            return reinterpret_cast<const sockaddr_in&>(m_storage).sin_addr.s_addr;
        }
        return 0;
    }

    /** Returns the raw network-order port (any family). */
    uint16_t port_raw() const {
        switch (m_storage.ss_family) {
            case AF_INET:
            return reinterpret_cast<const sockaddr_in&>(m_storage).sin_port;
            case AF_INET6:
            return reinterpret_cast<const sockaddr_in6&>(m_storage).sin6_port;
            default:
            return 0;
        }
    }

    uint16_t port() const {
        switch (m_storage.ss_family) {
            case AF_INET:
            return ntohs(reinterpret_cast<const sockaddr_in&>(m_storage).sin_port);
            case AF_INET6:
            return ntohs(reinterpret_cast<const sockaddr_in6&>(m_storage).sin6_port);
            default:
            return 0;
        }
    }

    /** Return the IP portion only (no port). */
    std::string ip_string() const {
        char buf[INET6_ADDRSTRLEN]{};
        switch (m_storage.ss_family) {
            case AF_INET: {
                const auto& v4 = reinterpret_cast<const sockaddr_in&>(m_storage);
                inet_ntop(AF_INET, &v4.sin_addr, buf, sizeof(buf));
                break;
            }
            case AF_INET6: {
                const auto& v6 = reinterpret_cast<const sockaddr_in6&>(m_storage);
                inet_ntop(AF_INET6, &v6.sin6_addr, buf, sizeof(buf));
                break;
            }
            default:
            return "<unknown>";
        }
        return std::string(buf);
    }

    /** Human-readable "ip:port" (or "[ip]:port" for IPv6). */
    std::string to_string() const {
        if (m_storage.ss_family == AF_INET6) {
            return std::format("[{}]:{}", ip_string(), port());
        }
        return std::format("{}:{}", ip_string(), port());
    }

    bool operator==(const Address& other) const {
        if (m_storage.ss_family != other.m_storage.ss_family) return false;

        switch (m_storage.ss_family) {
            case AF_INET: {
                const auto& a = reinterpret_cast<const sockaddr_in&>(m_storage);
                const auto& b = reinterpret_cast<const sockaddr_in&>(other.m_storage);
                return a.sin_port == b.sin_port
                && a.sin_addr.s_addr == b.sin_addr.s_addr;
            }
            case AF_INET6: {
                const auto& a = reinterpret_cast<const sockaddr_in6&>(m_storage);
                const auto& b = reinterpret_cast<const sockaddr_in6&>(other.m_storage);
                return a.sin6_port == b.sin6_port
                && std::memcmp(&a.sin6_addr, &b.sin6_addr, sizeof(in6_addr)) == 0;
            }
            default:
            return std::memcmp(&m_storage, &other.m_storage, sizeof(m_storage)) == 0;
        }
    }

    bool operator!=(const Address& other) const { return !(*this == other); }

    /**
        * Retained for source compatibility. Prefer operator==.
        */
    bool equals(const Address& other) const { return *this == other; }
};



}
template<>
struct std::hash<tw::net::Address> {
    std::size_t operator()(const tw::net::Address& addr) const noexcept {
        // FNV-style combine of family + port + address bytes
        std::size_t h = std::hash<uint16_t>{}(addr.family());
        h ^= std::hash<uint16_t>{}(addr.port_raw()) + 0x9e3779b9 + (h << 6) + (h >> 2);

        switch (addr.family()) {
            case AF_INET:
                h ^= std::hash<uint32_t>{}(addr.ipv4_addr_raw()) + 0x9e3779b9 + (h << 6) + (h >> 2);
                break;
            case AF_INET6: {
                const auto& s = reinterpret_cast<const sockaddr_in6&>(addr.storage());
                const auto* bytes = reinterpret_cast<const uint8_t*>(&s.sin6_addr);
                for (int i = 0; i < 16; ++i) {
                    h ^= std::hash<uint8_t>{}(bytes[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
                }
                break;
            }
            default:
                break;
        }
        return h;
    }
};
