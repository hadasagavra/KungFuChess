#pragma once

#include <string>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "rules/include/piece_rules.hpp"

namespace kfc::rules {

// The result of validating a single move. reason is always present: exactly
// "ok" when the move is valid, otherwise a stable, machine-readable token.
struct MoveValidation {
    bool isValid;
    std::string reason;
};

// Map a piece Kind to its stateless movement rule. Returns a reference to a
// shared stateless instance (no ownership transfer).
const PieceRules& ruleFor(model::Kind kind);

// Stateless engine that answers "is this move currently legal on this board?".
// It only inspects the board; it never moves, removes, or mutates anything, and
// it has no notion of check, mate, castling, en passant, promotion, or
// game-over (those live elsewhere).
class RuleEngine {
public:
    MoveValidation validateMove(const model::Board& board,
                                model::Position source,
                                model::Position destination) const;
};

}  // namespace kfc::rules
