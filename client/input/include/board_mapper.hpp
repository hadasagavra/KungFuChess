#pragma once

#include <optional>

#include "shared/logic/model/include/position.hpp"

namespace kfc::input {

// Maps a pixel in the rendered frame to the board cell under it. Display-coupled
// by nature, and deliberately free of game rules: it answers "which square was
// clicked", never "may that square be used".
//
// The board is not necessarily at the frame's top-left -- side panels push it
// across -- so the mapper is told where the board starts and how big a cell is.
// Those come from the same layout the renderer draws with, so a click and the
// square drawn under the cursor can never disagree.
class BoardMapper {
public:
    BoardMapper(int width, int height, int cellPx, int originX, int originY);

    std::optional<model::Position> toCell(int x, int y) const;

private:
    int width_;
    int height_;
    int cellPx_;
    int originX_;
    int originY_;
};

}  // namespace kfc::input
