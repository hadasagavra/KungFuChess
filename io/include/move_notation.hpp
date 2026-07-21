#pragma once

#include <string>

#include "model/include/game_event.hpp"
#include "model/include/position.hpp"

namespace kfc::io {

// Algebraic notation: the textual form of a move, e.g. "e4", "Nxc6", "exd4".
// This is serialization, the same kind of job BoardPrinter does for a whole
// board -- it turns domain data into text and decides nothing about the game.
// It reads the piece letters from piece_codec rather than listing them again.
//
// boardHeight is needed because ranks count up from the bottom while rows count
// down from the top; the board's own size is what relates the two.

// The name of a square, e.g. {0, 4} on an 8-high board -> "e8".
std::string squareName(model::Position cell, int boardHeight);

// The notation for one logged move. A jump has no standard chess spelling, so it
// is written with a marker prefix on the square the piece jumped in place on.
std::string moveNotation(const model::MoveEvent& move, int boardHeight);

}  // namespace kfc::io
