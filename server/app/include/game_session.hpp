#pragma once

#include <map>
#include <optional>

#include "server/net/include/message_transport.hpp"
#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/io/include/wire_message.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::server {

// The authoritative host of one game. It owns the single GameEngine, gives each
// client a colour, turns the commands clients send into engine calls, and
// broadcasts what the engine reports. It is the Server layer and holds no game
// rules of its own: legality is the engine's answer, and the one judgment the
// session does make -- may THIS client move THAT colour -- is authorization, not
// a chess rule.
//
// It speaks in typed WireMessages, never in wire text: decoding and encoding
// happen at the io boundary, and the session routes messages with std::visit.
class GameSession {
public:
    GameSession(model::Board& board, MessageTransport& transport);

    // Seat a client: assign white, then black, then spectator, and tell it which.
    void addClient(ClientId client);

    // A message arrived from a client. Only a command means anything here; a
    // client has no standing to send the server's own answers, so those are
    // ignored. Malformed text is dropped -- expected on a network, not an error.
    void handleMessage(ClientId client, const std::string& text);

    // Advance the clock and broadcast the resulting frame to every client.
    void tick(int deltaMs);

private:
    // The colour a client plays, or nullopt if it is a spectator or unknown.
    std::optional<model::Color> colorOf(ClientId client) const;
    // The colour the next client should get: white, then black, then none.
    std::optional<model::Color> assignColor() const;
    // Apply a command whose sender owns the colour it names.
    void handleCommand(ClientId client, const io::PlayerCommand& command);

    model::Board& board_;
    engine::GameEngine engine_;
    MessageTransport& transport_;
    std::map<ClientId, std::optional<model::Color>> roles_;
};

}  // namespace kfc::server
