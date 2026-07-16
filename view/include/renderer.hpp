#pragma once

#include "img.hpp"
#include "model/include/piece.hpp"
#include "view/include/game_snapshot.hpp"
#include "view/include/render_config.hpp"

namespace kfc::view {

// Display only. The Renderer paints a read-only GameSnapshot onto an image via
// the provided Img/OpenCV wrapper. It holds no game rules and never mutates
// domain state -- it draws the board and each piece's sprite, nothing more.
class Renderer {
public:
    explicit Renderer(RenderConfig config);

    // Compose one frame from the snapshot and return it. Does not display: the
    // caller decides whether to save or show, keeping the render loop
    // non-blocking (Img::show() blocks on a keypress).
    Img renderFrame(const GameSnapshot& snapshot) const;

private:
    Img loadBoardBackground(int widthCells, int heightCells) const;
    Img loadSprite(model::Kind kind, model::Color color, model::State state,
                   int frame) const;
    void drawPieceOn(Img& frame, const PieceView& piece) const;
    // Shades each legal-move hint cell with a translucent yellow fill. Drawn
    // after the board but before the pieces, so a piece (e.g. a capturable enemy)
    // stays visible on top of its highlight.
    void drawHighlightsOn(Img& frame, const GameSnapshot& snapshot) const;
    // Draws the cooldown indicator over a resting piece: a translucent shade
    // whose top edge descends down the cell as the rest completes. No-op for a
    // piece that is not resting.
    void drawCooldownOn(Img& frame, const PieceView& piece) const;

    RenderConfig config_;
};

}  // namespace kfc::view
