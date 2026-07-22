#pragma once

#include <functional>
#include <string>
#include <utility>

#include "server/net/include/message_transport.hpp"

namespace kfc::net {

// The server half of a same-process link: a MessageTransport that hands every
// message the server emits to one client sink. In loopback there is a single
// window standing in for both seats, so per-client sends and broadcasts alike go
// to the same place -- the sink delivers them to the in-process RemoteGame.
//
// This is why the client target links the server: loopback IS the server running
// inside the client, and this class is the wire between them.
class LoopbackTransport : public server::MessageTransport {
public:
    using ClientSink = std::function<void(const std::string&)>;

    void setClientSink(ClientSink sink) { clientSink_ = std::move(sink); }

    void send(server::ClientId, const std::string& message) override {
        deliver(message);
    }
    void broadcast(const std::string& message) override { deliver(message); }

private:
    void deliver(const std::string& message) const {
        if (clientSink_) clientSink_(message);
    }

    ClientSink clientSink_;
};

}  // namespace kfc::net
