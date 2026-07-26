#pragma once

#include <optional>
#include <vector>

#include "server/net/include/message_transport.hpp"

namespace kfc::server {

// The Play queue: it holds players who asked for any opponent and pairs two of
// them once their ratings are within range. It knows nothing of rooms, sockets,
// or games -- it only decides who plays whom -- so the lobby owns one and turns
// its answers into rooms. Time is fed in from outside (tick), so it needs no
// clock of its own and is fully deterministic to test.
class Matchmaker {
public:
    // Look for a waiting opponent within the Elo range of `rating`. If one is
    // waiting it is removed and returned (the caller should room the two). If not,
    // the seeker joins the queue and nullopt is returned. A client already in the
    // queue is left as-is.
    std::optional<ClientId> seek(ClientId client, int rating);

    // Remove a client from the queue (it cancelled or left).
    void cancel(ClientId client);

    // Advance every waiter's clock by deltaMs and return -- and remove -- those
    // whose wait has run out, so the caller can tell them no match was found.
    std::vector<ClientId> tick(int deltaMs);

    bool isWaiting(ClientId client) const;

private:
    struct Seeker {
        ClientId client;
        int rating;
        int waitedMs;
    };
    std::vector<Seeker> queue_;
};

}  // namespace kfc::server
