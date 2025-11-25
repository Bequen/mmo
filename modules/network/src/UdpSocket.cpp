#include "UdpSocket.hpp"

#include <sys/socket.h>

namespace tw::net {


UdpSocket::UdpSocket(Address address) :
    buffer(4096) 
{
    const int domain = AF_INET;

    socket_fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
    if(socket_fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    
    sockaddr_in socket_addr = address.address;

    if(bind(socket_fd, (sockaddr*)&socket_addr, sizeof(socket_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
}


void UdpSocket::send_bytes(const Address& address, const std::vector<uint8_t>& mesg) {
    int result = sendto(socket_fd, 
            (const void*)mesg.data(), mesg.size(), 
            0, 
            (sockaddr*)&address.address, sizeof(sockaddr_in));

    if(result <= 0 || result != mesg.size()) {
        throw std::runtime_error("Failed to send bytes: " + std::string(strerror(errno)));
    }
}


std::pair<Address, std::vector<std::uint8_t>> UdpSocket::receive_bytes(size_t max_packet_size) {
    sockaddr_storage sender_address = {};
    socklen_t address_length = sizeof(sender_address);

    size_t result = recvfrom(socket_fd, (void*)buffer.data(), max_packet_size,  
                0, (struct sockaddr*)&sender_address, 
                &address_length); 

    if(result <= 0) {
        throw std::runtime_error("Failed to receive bytes: " + std::string(strerror(errno)));
    }

    // null terminated
    buffer[result] = {0};

    return std::make_pair(Address(sender_address), buffer);
}

}
