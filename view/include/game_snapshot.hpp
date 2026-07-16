#pragma once

#include <cstdint>
#include <vector>

#include "model/include/piece.hpp"
#include "view/include/animation_config.hpp"

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

// A read-only description of one piece to draw: what it is, where (in pixels)
// the top-left of its sprite goes, and which sprite frame to draw. Holds no live
// domain object, so the view cannot accidentally mutate game state through it.
//
// id is the piece's stable identity, so the Animator can track a piece's frame
// across frames. frame is the sprite number to draw (1-based); the seam leaves
// it at the first frame and the Animator fills the animated value.
struct PieceView {
    model::Kind kind;
    model::Color color;
    model::State state;
    PixelPoint position;
    std::uint32_t id = 0;
    int frame = firstSpriteFrame;
};

// The read-only contract the Renderer draws. Board dimensions are in CELLS;
// piece positions are already in PIXELS (the seam converts cells to pixels).
struct GameSnapshot {
    int boardWidth;
    int boardHeight;
    std::vector<PieceView> pieces;
};

}  // namespace kfc::view
