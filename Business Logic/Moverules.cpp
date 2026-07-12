#include "MoveRules.hpp"

#include <cstdlib>

namespace kfc::logic {

namespace {

bool isShapeLegal(PieceType piece, Position from, Position to) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    if (dr == 0 && dc == 0) return false;

    int adr = std::abs(dr);
    int adc = std::abs(dc);

    switch (piece) {
        case PieceType::King:   return adr <= 1 && adc <= 1;
        case PieceType::Rook:   return dr == 0 || dc == 0;
        case PieceType::Bishop: return adr == adc;
        case PieceType::Queen:  return dr == 0 || dc == 0 || adr == adc;
        case PieceType::Knight: return (adr == 1 && adc == 2) || (adr == 2 && adc == 1);
        case PieceType::Pawn:   return false;  // pawns handled separately
    }
    return false;
}

bool requiresClearPath(PieceType piece) {
    return piece == PieceType::Rook || piece == PieceType::Bishop || piece == PieceType::Queen;
}

bool isPathClear(const Board& board, Position from, Position to) {
    int dr = to.row - from.row;
    int dc = to.col - from.col;
    int stepR = (dr > 0) - (dr < 0);
    int stepC = (dc > 0) - (dc < 0);

    Position cur{from.row + stepR, from.col + stepC};
    while (cur.row != to.row || cur.col != to.col) {
        if (!board.isEmpty(cur)) return false;
        cur.row += stepR;
        cur.col += stepC;
    }
    return true;
}

int pawnDirection(char color) {
    return (color == kWhiteColor) ? -1 : 1;
}

int pawnStartRow(const Board& board, char color) {
    return (color == kWhiteColor) ? board.height() - 1 : 0;
}

bool isPawnMoveLegal(const Board& board, char color, Position from, Position to) {
    int dir = pawnDirection(color);
    int dr = to.row - from.row;
    int dc = to.col - from.col;

    if (dr == dir && std::abs(dc) == 1) {
        return !board.isEmpty(to) && board.colorAt(to) != color;
    }

    if (dc == 0 && dr == dir) {
        return board.isEmpty(to);
    }

    if (dc == 0 && dr == 2 * dir && from.row == pawnStartRow(board, color)) {
        Position between{from.row + dir, from.col};
        return board.isEmpty(between) && board.isEmpty(to);
    }

    return false;
}

}  // namespace

bool isMoveLegal(const Board& board, PieceType piece, char color, Position from, Position to) {
    if (piece == PieceType::Pawn) {
        return isPawnMoveLegal(board, color, from, to);
    }
    if (!isShapeLegal(piece, from, to)) return false;
    if (requiresClearPath(piece) && !isPathClear(board, from, to)) return false;
    return true;
}

}  // namespace kfc::logic