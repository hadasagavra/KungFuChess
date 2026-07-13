#include "input/include/board_mapper.hpp"

namespace kfc::input {

BoardMapper::BoardMapper(int width, int height)
    : width_(width), height_(height) {}

std::optional<model::Position> BoardMapper::toCell(int x, int y) const {
    if (x < 0 || y < 0) return std::nullopt;

    int col = x / kCellSize;
    int row = y / kCellSize;
    if (row >= height_ || col >= width_) return std::nullopt;

    return model::Position{row, col};
}

}