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
    engine_.requestMove(*selected_, *cell);
    selected_.reset();     // clear after every in-board second click, legal or not
}

}  // namespace kfc::input
