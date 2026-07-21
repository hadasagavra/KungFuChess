#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "shared/logic/model/include/piece.hpp"
#include "client/view/include/animation_config.hpp"

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
//
// restProgress is how far a resting piece is through its cooldown (0..1, 1 =
// done); it drives the cooldown overlay. It is only meaningful when
// state == Resting and stays 0 otherwise.
struct PieceView {
    model::Kind kind;
    model::Color color;
    model::State state;
    PixelPoint position;
    std::uint32_t id = 0;
    int frame = firstSpriteFrame;
    double restProgress = 0.0;
};

// One row of a player's move table, already written out as text. The seam does
// the formatting so the Renderer only ever places strings -- it never learns
// what a move is, how a clock is written, or what the notation means.
struct MoveRow {
    std::string time;
    std::string move;
};

// Everything shown in one player's side panel.
struct PlayerPanel {
    std::string name;
    int score = 0;
    std::vector<MoveRow> moves;
};

// The read-only contract the Renderer draws. Board dimensions are in CELLS;
// piece positions are already in PIXELS (the seam converts cells to pixels).
struct GameSnapshot {
    int boardWidth;
    int boardHeight;
    // Top-left pixel of the board within the frame. Non-zero once side panels
    // take up room to the left of it.
    PixelPoint boardOrigin{0, 0};
    std::vector<PieceView> pieces;
    // Cells to shade as legal-move hints, each the top-left pixel of a cell. The
    // seam fills these from the engine's legal-destination query; the Renderer
    // paints them. Empty when nothing is selected.
    std::vector<PixelPoint> highlights;
    PlayerPanel whitePanel;
    PlayerPanel blackPanel;
};

}  // namespace kfc::view
