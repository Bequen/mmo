#pragma once

#include <cerrno>
#include <cstring>
#include <string>
namespace tw::net {


enum NetworkErrorType {
    MESSAGE_TOO_LONG = 90,
    ADDRESS_FAMILY_NOT_SUPPORTED = 97,
    BAD_FILE_DESCRIPTOR = 9,
    CONNECTION_RESET = 104,
    WOULD_BLOCK = 11,
    INTERRUPED = 4,
    INVALID_ARGUMENT = 22,
    NOT_CONNECTED = 107,
    NOT_SOCKET = 88,
    OPERATION_NOT_SUPPORTED = 95,
    TIMED_OUT = 110,
    IO_ERROR = 5,
    NO_BUFFER_SPACE = 105,
    NOT_ENOUGH_MEMORY = 12,
    DESTINATION_ADDRESS_REQUIRED = 89,
    BROKEN_PIPE = 32
};

struct NetworkError {
    NetworkErrorType m_type;

public:
    static NetworkError from_errno(int err) {
        return { static_cast<NetworkErrorType>(err) };
    }

    std::string message() const {
        switch (m_type) {
            case BAD_FILE_DESCRIPTOR:
                return "The socket is not a valid file descriptor";
            case CONNECTION_RESET:
                return "A connection was forcibly closed by a peer.";
            case INTERRUPED:
                return "The function was interrupted by a signal that was caught, before any data was available.";
            case INVALID_ARGUMENT:
                return "The MSG_OOB flag is set and no out-of-band data is available.";
            case NOT_CONNECTED:
                return "A function is attempted on connection-mode socket that is not connected.";
            case NOT_SOCKET:
                return "Socket operation on non-socket.";
            case OPERATION_NOT_SUPPORTED:
                return "The specified flags are not supported for this socket type or protocol.";
            case TIMED_OUT:
                return "The connection timed out during connection establishment, or due to a transmission timeout on active connection.";
            case IO_ERROR:
                return "An I/O error occurred while reading from or writing to the file system.";
            case NO_BUFFER_SPACE:
                return "Insufficient resources were available in the system to perform the operation.";
            case NOT_ENOUGH_MEMORY:
                return "Insufficient memory was available to complete the operation.";
            case DESTINATION_ADDRESS_REQUIRED:
                return "The destination address is required for this operation.";
            case BROKEN_PIPE:
                return "The write end of a pipe or socket has been closed.";
            default:
                return std::string(strerror(static_cast<int>(m_type)));
        }
    }
};

}
