#pragma once

#include <set>
#include <memory>
#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

namespace kfc::rules {

enum class MoveReason {
    // Movement-legality outcomes, produced by RuleEngine::validateMove.
    Ok,
    OutsideBoard,
    EmptySource,
    FriendlyDestination,
    IllegalPieceMove,
    // Real-time / game outcomes, produced by the engine and carried through
    // engine::MoveResult (the engine forwards the legality reasons above as-is).
    GameOver,
    MotionInProgress,
    NotIdle,
    NoPiece
};

struct MoveValidation {
    bool isValid;
    MoveReason reason;
};

class RuleEngine {
public:
    MoveValidation validateMove(const model::Board& board,
                                model::Position source,
                                model::Position destination) const;

    std::set<model::Position> getLegalDestinations(const model::Board& board,
                                                   model::Position source) const;
};

}  // namespace kfc::rules
