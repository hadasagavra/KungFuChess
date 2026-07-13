#include "rules/include/rule_engine.hpp"

#include <memory>
#include <set>
#include <stdexcept>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "rules/include/piece_rules.hpp"

namespace kfc::rules {

using model::Board;
using model::Kind;
using model::Piece;
using model::Position;

namespace {

constexpr const char* reasonOk = "ok";
constexpr const char* reasonOutsideBoard = "outside_board";
constexpr const char* reasonEmptySource = "empty_source";
constexpr const char* reasonFriendlyDestination = "friendly_destination";
constexpr const char* reasonIllegalPieceMove = "illegal_piece_move";

}  // namespace

const PieceRules& ruleFor(Kind kind) {
    static const RookRules rook;
    static const BishopRules bishop;
    static const QueenRules queen;
    static const KnightRules knight;
    static const KingRules king;
    static const PawnRules pawn;

    switch (kind) {
        case Kind::Rook:   return rook;
        case Kind::Bishop: return bishop;
        case Kind::Queen:  return queen;
        case Kind::Knight: return knight;
        case Kind::King:   return king;
        case Kind::Pawn:   return pawn;
    }
    throw std::invalid_argument("ruleFor: unknown piece kind");
}

MoveValidation RuleEngine::validateMove(const Board& board, Position source,
                                        Position destination) const {
    if (!board.isInsideBounds(source) || !board.isInsideBounds(destination)) {
        return {false, reasonOutsideBoard};
    }

    const std::shared_ptr<Piece> mover = board.getPieceAt(source);
    if (!mover) {
        return {false, reasonEmptySource};
    }

    const std::shared_ptr<Piece> target = board.getPieceAt(destination);
    if (target && target->getColor() == mover->getColor()) {
        return {false, reasonFriendlyDestination};
    }

    const std::set<Position> destinations =
        ruleFor(mover->getKind()).legalDestinations(board, *mover);
    if (destinations.count(destination) == 0) {
        return {false, reasonIllegalPieceMove};
    }

    return {true, reasonOk};
}

}  // namespace kfc::rules
