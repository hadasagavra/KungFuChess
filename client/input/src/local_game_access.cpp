#include "client/input/include/local_game_access.hpp"

namespace kfc::input {

LocalGameAccess::LocalGameAccess(engine::GameEngine& engine) : engine_(engine) {}

std::optional<model::Piece> LocalGameAccess::pieceAt(model::Position cell) const {
    return engine_.getSnapshot().pieceAt(cell);
}

void LocalGameAccess::requestMove(model::Position from, model::Position to) {
    // The outcome (accepted or not) is not returned: the caller learns it from
    // the state that follows, exactly as it would over a network.
    engine_.requestMove(from, to);
}

void LocalGameAccess::requestJump(model::Position cell) {
    engine_.requestJump(cell);
}

}  // namespace kfc::input
