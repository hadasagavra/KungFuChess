#pragma once

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "client/input/include/game_access.hpp"
#include "shared/bus/include/event_bus.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/realtime/include/motion.hpp"
#include "shared/logic/rules/include/rule_engine.hpp"

namespace kfc::net {

// The client's whole knowledge of the game. It never runs the rules of time: it
// keeps a replica board and the last frame of motions/cooldowns, rebuilt from
// the state the server broadcasts, and it publishes the events the server relays
// onto a local bus so this client's own MoveLog and ScoreBoard record them
// unchanged. It sends commands but never decides their fate -- the answer comes
// back as the next state.
//
// It is colour-agnostic on the way in (state is the same for everyone) and
// colour-aware on the way out: a command carries the colour of the piece being
// moved, read from the replica. Who is allowed to issue that colour is settled
// downstream -- by the sink's wiring in loopback (a seat per colour) and by the
// server's ownership check either way.
//
// It also keeps the colours the server assigned it (ownColors_): a networked
// client gets one (or none, as a spectator); a loopback client gets both, since
// the one window is seated as each colour in turn. Highlights and outgoing
// commands are gated by that set, so a spectator is view-only and a player never
// highlights or moves the opponent's pieces. The server stays the authority; the
// gate only spares it commands it would refuse and the GUI hints it would not.
class RemoteGame : public input::GameAccess {
public:
    // Sends one command message on behalf of the given colour. The colour lets a
    // loopback wiring pick the right seat; a single-player socket ignores it and
    // lets the server's ownership check stand.
    using CommandSink = std::function<void(model::Color mover,
                                           const std::string& message)>;

    RemoteGame(model::Board replica, CommandSink sink);

    // Apply one message the server sent: a state frame refreshes the replica; a
    // move or capture event is republished locally; a role assignment records
    // which colour(s) this client may command; a roster records the player names
    // and ratings; a rejection records why a login was refused; anything else is
    // ignored.
    void receive(const std::string& message);

    engine::GameSnapshot getSnapshot() const;
    std::set<model::Position> legalDestinationsFor(model::Position source) const;
    bus::EventBus& events() { return bus_; }

    // The player names the server last broadcast, empty until a roster arrives.
    const std::string& whiteName() const { return whiteName_; }
    const std::string& blackName() const { return blackName_; }
    // The player ratings the server last broadcast, absent until a roster arrives.
    std::optional<int> whiteRating() const { return whiteRating_; }
    std::optional<int> blackRating() const { return blackRating_; }
    // Why the server refused this client's login, if it did (bad password).
    const std::optional<std::string>& authError() const { return authError_; }

    // -- Lobby: the Home screen drives these ------------------------------------
    // Ask the server for any opponent (Play), or stop asking.
    void seekGame();
    void cancelSeek();
    // Open a fresh room, or join one by id.
    void createRoom();
    void joinRoom(const std::string& roomId);

    // True while a Play search is outstanding (before a match or a timeout).
    bool isSearching() const { return searching_; }
    // True once the server has put this client in a room -- time to draw the game.
    bool isInGame() const { return inGame_; }
    // The room this client is in, shown at the top of the screen (empty in Home).
    const std::string& roomId() const { return roomId_; }
    // A one-line status for the Home screen ("Searching…", "Couldn't find…", an
    // error), or empty when there is nothing to say.
    const std::string& statusMessage() const { return statusMessage_; }
    // Seconds until an absent opponent forfeits, while one is disconnected.
    std::optional<int> opponentCountdown() const { return opponentCountdown_; }

    std::optional<model::Piece> pieceAt(model::Position cell) const override;
    void requestMove(model::Position from, model::Position to) override;
    void requestJump(model::Position cell) override;

private:
    // Whether a piece on this cell is mid-move or resting right now, read from the
    // last frame -- the replica alone cannot say, since it carries no piece state.
    bool isBusy(model::Position cell) const;
    // Encode and send a command for the piece on `from` going to `to` (to == from
    // is a jump). Does nothing if no piece sits there to name the mover.
    void sendCommand(model::Position from, model::Position to);
    // Send a lobby message (no piece colour). The sink takes a colour to pick a
    // loopback seat, but lobby play is networked-only, where the sink ignores it,
    // so any colour serves here.
    void sendLobby(const io::WireMessage& message);

    // Whether this client is allowed to command the given colour right now.
    bool mayCommand(model::Color color) const {
        return ownColors_.count(color) != 0;
    }

    model::Board replica_;
    CommandSink sink_;
    bus::EventBus bus_;
    rules::RuleEngine ruleEngine_;
    bool isOver_ = false;
    std::vector<realtime::MotionState> motions_;
    std::vector<realtime::CooldownState> cooldowns_;
    std::set<model::Color> ownColors_;
    std::string whiteName_;
    std::string blackName_;
    std::optional<int> whiteRating_;
    std::optional<int> blackRating_;
    std::optional<std::string> authError_;
    bool searching_ = false;
    bool inGame_ = false;
    std::string roomId_;
    std::string statusMessage_;
    std::optional<int> opponentCountdown_;
};

}  // namespace kfc::net
