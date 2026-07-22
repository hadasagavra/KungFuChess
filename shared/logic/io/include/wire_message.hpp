#pragma once

#include <optional>
#include <string>
#include <variant>

#include "shared/logic/io/include/command_notation.hpp"
#include "shared/logic/model/include/game_event.hpp"
#include "shared/logic/model/include/piece.hpp"

namespace kfc::io {

// Which colour a client was given, or nothing at all when it joined as a
// spectator. Sent by the server to a client the moment it connects.
struct RoleAssignment {
    std::optional<model::Color> color;  // nullopt == spectator
};

// A whole game frame on its way to a client. It carries the state_codec text as
// an opaque payload rather than a decoded board: a board is decoded into the
// client's own replica (state_codec fills a caller's Board in place), so pulling
// it into this value would either copy a board or fight that seam. At the
// message layer it is enough that this is, by type, a state update.
struct StateUpdate {
    std::string frame;
};

// Every kind of message that can travel between a client and the server, as one
// strongly typed sum. Nothing above the wire deals in tag strings or substrings:
// a message is one of these alternatives, and std::visit forces every kind to be
// handled. Two of the alternatives are the very domain events the engine already
// publishes (MoveEvent, CapturedPiece); a command is the request a client makes;
// the other two are the server's answers.
using WireMessage = std::variant<PlayerCommand, RoleAssignment, model::MoveEvent,
                                 model::CapturedPiece, StateUpdate>;

// The one boundary where a typed message becomes wire text and back. The tag
// strings that name each kind on the wire live only inside these two functions;
// they are an implementation detail of this serialization, never seen above it.
std::string encode(const WireMessage& message, int boardHeight);
std::optional<WireMessage> decode(const std::string& text, int boardHeight);

}  // namespace kfc::io
