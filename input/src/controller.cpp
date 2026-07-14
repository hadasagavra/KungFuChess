#include "input/include/controller.hpp"

namespace kfc::input {

Controller::Controller(engine::GameEngine& engine, BoardMapper mapper)
    : engine_(engine), mapper_(mapper) {}

void Controller::handleClick(int x, int y) {
    std::optional<model::Position> cell = mapper_.toCell(x, y);
    if (!selected_) {
        handleFirstClick(cell);
    } else {
        handleSecondClick(cell);
    }
}

const std::optional<model::Position>& Controller::selection() const {
    return selected_;
}

void Controller::handleFirstClick(std::optional<model::Position> cell) {
    if (!cell) return;                                  // off-board, no selection: ignore
    if (!engine_.getSnapshot().pieceAt(*cell)) return;  // empty cell: ignore
    selected_ = cell;
}

void Controller::handleSecondClick(std::optional<model::Position> cell) {
    if (!cell) {            // off-board with a selection: cancel, send nothing
        selected_.reset();
        return;
    }
    // Clicking one of your own pieces re-selects it rather than moving onto it.
    const std::optional<model::Piece> selectedPiece =
        engine_.getSnapshot().pieceAt(*selected_);
    const std::optional<model::Piece> target = engine_.getSnapshot().pieceAt(*cell);
    if (target && selectedPiece && target->getColor() == selectedPiece->getColor()) {
        selected_ = cell;
        return;
    }
    engine_.requestMove(*selected_, *cell);
    selected_.reset();     // clear after every in-board second click, legal or not
}

void Controller::handleJump(int x, int y) {
    std::optional<model::Position> cell = mapper_.toCell(x, y);
    if (cell) engine_.requestJump(*cell);
}
}  // namespace kfc::input
