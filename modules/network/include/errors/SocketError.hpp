#pragma once

#include "Address.hpp"
#include "NetworkError.hpp"

namespace tw::net {

class SocketError {
    Address m_address;

    NetworkError m_error;

public:
    SocketError(Address address, NetworkError network_error);
};

}
