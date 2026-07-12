#pragma once

#include "Board.hpp"
#include "PieceTypes.hpp"

namespace kfc::logic {

bool isMoveLegal(const Board& board, PieceType piece, char color, Position from, Position to);

}  // namespace kfc::logic