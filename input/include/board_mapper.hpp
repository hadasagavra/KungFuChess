#pragma once

#include <optional>

#include "model/include/position.hpp"

namespace kfc::input {

inline constexpr int kCellSize = 100;

class BoardMapper {
public:
    BoardMapper(int width, int height);

    std::optional<model::Position> toCell(int x, int y) const;

private:
    int width_;
    int height_;
};

}