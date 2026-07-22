#pragma once

#include <cstdint>
#include <string>

namespace kfc::server {

// A client's handle for the length of its connection. The transport hands them
// out; the session uses them to say which client a message is for.
using ClientId = std::uint32_t;

// How the session reaches the clients, and nothing more. It moves opaque strings
// and names no game concept, so the session can be driven by a real WebSocket, a
// same-process loopback, or a test double without changing a line. This is the
// seam that keeps the whole session testable without a socket.
class MessageTransport {
public:
    virtual ~MessageTransport() = default;

    // Deliver one message to one client, and to every client, respectively.
    virtual void send(ClientId client, const std::string& message) = 0;
    virtual void broadcast(const std::string& message) = 0;
};

}  // namespace kfc::server
