#pragma once

#include <string>

namespace kfc::net {

// The narrow seam the Home-screen controller drives: the lobby's outgoing actions
// plus the one piece of state it displays. A LobbyController depends on this and
// nothing more -- exactly as input::Controller depends on input::GameAccess
// rather than the whole game -- so it neither knows nor cares whether a real
// socket or a test double sits behind it.
class LobbyAccess {
public:
    virtual ~LobbyAccess() = default;

    // Find any opponent by rating (Play), or stop looking.
    virtual void seekGame() = 0;
    virtual void cancelSeek() = 0;
    // Open a fresh room, or join one by id.
    virtual void createRoom() = 0;
    virtual void joinRoom(const std::string& roomId) = 0;

    // Whether a Play search is outstanding, and a one-line status for the Home
    // screen ("Searching...", "Couldn't find...", an error) or empty.
    virtual bool isSearching() const = 0;
    virtual std::string statusMessage() const = 0;
};

}  // namespace kfc::net
