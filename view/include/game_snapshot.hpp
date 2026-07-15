#pragma once

#include <vector>

#include "model/include/piece.hpp"

namespace kfc::view {

// A single pixel coordinate on the rendered frame (top-left origin).
struct PixelPoint {
    int x;
    int y;
};

inline bool operator==(const PixelPoint& a, const PixelPoint& b) {
    return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const PixelPoint& a, const PixelPoint& b) {
    return !(a == b);
}

// A read-only description of one piece to draw: what it is and where (in
// pixels) the top-left of its sprite goes. Holds no live domain object, so the
// view cannot accidentally mutate game state through it.
struct PieceView {
    model::Kind kind;
    model::Color color;
    model::State state;
    PixelPoint position;
};

// The read-only contract the Renderer draws. Board dimensions are in CELLS;
// piece positions are already in PIXELS (the seam converts cells to pixels).
struct GameSnapshot {
    int boardWidth;
    int boardHeight;
    std::vector<PieceView> pieces;
};

}  // namespace kfc::view
