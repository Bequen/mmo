#pragma once

#include "Address.hpp"

#include <bits/stdc++.h> 
#include <cstddef>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 


namespace tw::net {

class UdpSocket {
private:
    int socket_fd = 0;
    std::vector<std::uint8_t> buffer;

public:
    UdpSocket(Address address);

    ~UdpSocket() {
        close(socket_fd);
    }

    void send_bytes(const Address& address, const std::vector<uint8_t>& mesg);

    std::pair<Address, std::vector<std::uint8_t>> receive_bytes(size_t max_packet_size);
};

}
