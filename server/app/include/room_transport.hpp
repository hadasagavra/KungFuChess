#pragma once

#include <set>
#include <string>

#include "server/net/include/message_transport.hpp"

namespace kfc::server {

// A MessageTransport scoped to one room's members. It wraps the real transport
// and the room's live member set: send() delegates straight through, and
// broadcast() reaches only this room's members rather than every connected
// client. This is what lets each room's GameSession stay unchanged -- it still
// broadcasts to "everyone", but "everyone" now means the room.
//
// It holds the member set by reference, so as a room gains and loses members its
// broadcasts follow without the transport being told.
class RoomTransport : public MessageTransport {
public:
    RoomTransport(MessageTransport& underlying, const std::set<ClientId>& members)
        : underlying_(underlying), members_(members) {}

    void send(ClientId client, const std::string& message) override {
        underlying_.send(client, message);
    }

    void broadcast(const std::string& message) override {
        for (const ClientId member : members_) {
            underlying_.send(member, message);
        }
    }

private:
    MessageTransport& underlying_;
    const std::set<ClientId>& members_;
};

}  // namespace kfc::server
