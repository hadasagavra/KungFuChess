#pragma once

#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::model {

// A move the engine accepted, described in pure domain terms. It deliberately
// carries no notation string and no formatted clock: turning it into "Nxc6" or
// "00:04.105" is serialization/display work that happens at the boundary, so
// this type stays usable by anything that cares about the move itself.
//
// timeMs is elapsed game time at the instant the command was accepted -- in a
// simultaneous game a move is "made" when it is ordered, not when it lands.
struct MoveEvent {
    int timeMs;
    Color player;
    Kind kind;
    Position from;
    Position to;
    bool isCapture;
    bool isJump;
};

// A piece that was taken off the board. The single description of a capture's
// victim, shared by every layer that needs one: the arbiter reports it upward in
// an ArrivalReport, the engine publishes it, and the ScoreBoard consumes it.
struct CapturedPiece {
    Kind kind;
    Color color;
};

}  // namespace kfc::model
