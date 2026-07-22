#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace kfc::net {

// A WebSocket connection to the server, from the client's side. It is the socket
// twin of the loopback link: RemoteGame sends command text through send() and
// receives the server's messages through the onMessage handler, exactly as it
// did in loopback. The asio/websocketpp headers are hidden behind a pimpl.
//
// Single-threaded, like the server: poll() pumps whatever the socket has ready
// and returns, so RemoteGame's bus is only touched from the frame loop.
class WebSocketClient {
public:
    // Begins connecting to ws://host:port. The connection completes during the
    // first poll()s; sends before it is open are dropped.
    WebSocketClient(const std::string& host, std::uint16_t port);
    ~WebSocketClient();

    void onMessage(std::function<void(const std::string&)> handler);

    // Handle any ready network events (handshake, reads) without blocking.
    void poll();

    // Send one message to the server, or drop it if not connected yet.
    void send(const std::string& message);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kfc::net
