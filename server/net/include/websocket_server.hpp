#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "server/net/include/message_transport.hpp"

namespace kfc::server {

// A MessageTransport backed by a real WebSocket endpoint. It is the socket twin
// of the loopback: it drops into a GameSession exactly where LoopbackTransport
// did, so nothing above it changes. The WebSocket stack (asio + websocketpp) is
// hidden behind a pimpl, so these heavy headers stay inside one .cpp and never
// reach a caller.
//
// Single-threaded by design: poll() runs whatever network events are ready and
// returns, so the session's EventBus (which is not thread-safe) is only ever
// touched from the one loop that calls poll() and tick().
class WebSocketServer : public MessageTransport {
public:
    explicit WebSocketServer(std::uint16_t port);
    ~WebSocketServer() override;

    // Called with a fresh ClientId when a client connects / with the sender's id
    // and the text when a message arrives. Set them before the first poll().
    void onClientConnected(std::function<void(ClientId)> handler);
    void onMessage(std::function<void(ClientId, const std::string&)> handler);

    // Called with a client's id when its connection closes (it left or dropped),
    // so the lobby can start a forfeit countdown or free a spectator.
    void onClientDisconnected(std::function<void(ClientId)> handler);

    // Handle any ready network events (accepts, reads) without blocking.
    void poll();

    void send(ClientId client, const std::string& message) override;
    void broadcast(const std::string& message) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kfc::server
