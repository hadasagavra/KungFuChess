#pragma once

#include "Board.h"
#include "PieceTypes.h"

namespace kfc::logic {

bool isMoveLegal(const Board& board, PieceType piece, char color, Position from, Position to);

}  // namespace kfc::logic