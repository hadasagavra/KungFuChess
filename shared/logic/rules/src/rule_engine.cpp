#include "shared/logic/rules/include/rule_engine.hpp"
#include "shared/logic/rules/include/piece_rules.hpp"

namespace kfc::rules {

using model::Board;
using model::Piece;
using model::Position;
using model::Kind;

namespace {


MoveValidation invalid(MoveReason reason) {
    return MoveValidation{false, reason};
}

MoveValidation valid() {
    return MoveValidation{true, MoveReason::Ok};
}

bool isFriendlyObstruction(const std::shared_ptr<Piece>& mover, 
                            const std::shared_ptr<Piece>& target) {
    return target && (target->getColor() == mover->getColor());
}

} // namespace

MoveValidation RuleEngine::validateMove(const Board& board, 
                                        Position source, 
                                        Position destination) const {
    if (!board.isInsideBounds(source) || !board.isInsideBounds(destination)) {
        return invalid(MoveReason::OutsideBoard);
    }

    const std::shared_ptr<Piece> mover = board.getPieceAt(source);
    if (!mover) {
        return invalid(MoveReason::EmptySource);
    }

    const std::shared_ptr<Piece> target = board.getPieceAt(destination);
    if (isFriendlyObstruction(mover, target)) {
        return invalid(MoveReason::FriendlyDestination);
    }

    const std::set<Position> destinations = getLegalDestinations(board, source);
    if (destinations.count(destination) == 0) {
        return invalid(MoveReason::IllegalPieceMove);
    }

    return valid();
}

std::set<Position> RuleEngine::getLegalDestinations(const Board& board, 
                                                    Position source) const {
    if (!board.isInsideBounds(source)) {
        return {};
    }

    const std::shared_ptr<Piece> mover = board.getPieceAt(source);
    if (!mover) {
        return {};
    }

    return ruleFor(mover->getKind()).legalDestinations(board, *mover);
}

} // namespace kfc::rules