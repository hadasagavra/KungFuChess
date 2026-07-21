#include "input/include/board_mapper.hpp"

namespace kfc::input {

BoardMapper::BoardMapper(int width, int height, int cellPx, int originX,
                         int originY)
    : width_(width),
      height_(height),
      cellPx_(cellPx),
      originX_(originX),
      originY_(originY) {}

std::optional<model::Position> BoardMapper::toCell(int x, int y) const {
    // Move into board-relative pixels first; anything left of or above the board
    // (a click on a side panel, say) falls outside it.
    const int boardX = x - originX_;
    const int boardY = y - originY_;
    if (boardX < 0 || boardY < 0) return std::nullopt;

    const int col = boardX / cellPx_;
    const int row = boardY / cellPx_;
    if (row >= height_ || col >= width_) return std::nullopt;

    return model::Position{row, col};
}

}  // namespace kfc::input
