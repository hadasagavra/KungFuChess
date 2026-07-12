#pragma once

#include <set>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::rules {

// Stateless strategy interface: given a read-only Board and a Piece, compute the
// set of squares that piece may move to under its movement pattern (including
// enemy-capture squares). Implementations store no state and never mutate the
// board, initiate moves, or perform captures -- they only calculate targets.
class PieceRules {
public:
    virtual ~PieceRules() = default;

    virtual std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const = 0;
};

// Horizontal and vertical sliding until an obstruction.
class RookRules : public PieceRules {
public:
    std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const override;
};

// Diagonal sliding until an obstruction.
class BishopRules : public PieceRules {
public:
    std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const override;
};

// Combination of rook and bishop sliding.
class QueenRules : public PieceRules {
public:
    std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const override;
};

// L-shaped jumps, ignoring obstructions.
class KnightRules : public PieceRules {
public:
    std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const override;
};

// One square in any direction.
class KingRules : public PieceRules {
public:
    std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const override;
};

// Forward one square (never a capture); capture one square diagonally forward.
// No initial double-step, no en passant, no promotion.
class PawnRules : public PieceRules {
public:
    std::set<model::Position> legalDestinations(
        const model::Board& board, const model::Piece& piece) const override;
};

}  // namespace kfc::rules
