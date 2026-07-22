#pragma once

#include <optional>
#include <string>

#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"

namespace kfc::io {

// One player's command as it travels from a client to the server: "WQe2e5" --
// the mover's colour, the piece kind, the source square, the destination square.
//
// A jump is written with from == to ("WQe2e2"). That is not a special case
// invented for the wire: the engine already publishes a jump as a move onto its
// own square (see GameEngine::requestJump), so the text says what the domain
// says and no reader needs a second convention.
//
// This describes what was *asked for*, never what is allowed. Whether the
// command is legal is the RuleEngine's answer, and nothing here anticipates it.
struct PlayerCommand {
    model::Color player;
    model::Kind kind;
    model::Position from;
    model::Position to;
};

// The colour letter is written upper case on the wire ("WQe2e5") while a board
// cell writes it lower case ("wQ"). Only the case differs: which letter stands
// for which colour is still piece_codec's single answer, and this converts case
// at the boundary rather than keeping a second table.
std::string encodeCommand(const PlayerCommand& command, int boardHeight);

// nullopt when the text is not a well-formed command. Malformed input is an
// expected event on a network, not an error: the server drops it and plays on.
std::optional<PlayerCommand> decodeCommand(const std::string& text,
                                           int boardHeight);

}  // namespace kfc::io
