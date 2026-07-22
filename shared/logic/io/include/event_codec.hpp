#pragma once

#include <optional>
#include <string>

#include "shared/logic/model/include/game_event.hpp"

namespace kfc::io {

// The wire form of the events the engine publishes, so the server can relay what
// happened and a client can rebuild it. Unlike a command (a client's request),
// an event is the server's authoritative account of the past -- so every field
// is encoded, timeMs included, and decode restores the exact struct that was
// sent. A client feeds the rebuilt events to its own MoveLog / ScoreBoard, which
// already take events from anyone (the engine, a replay, or a server).
//
// This is not move_notation: that renders "Nxc6" for a human to read and throws
// away the colour, the origin, and the clock. These round-trip.
std::string encodeMoveEvent(const model::MoveEvent& event, int boardHeight);
std::optional<model::MoveEvent> decodeMoveEvent(const std::string& text,
                                                int boardHeight);

std::string encodeCapturedPiece(const model::CapturedPiece& captured);
std::optional<model::CapturedPiece> decodeCapturedPiece(const std::string& text);

}  // namespace kfc::io
