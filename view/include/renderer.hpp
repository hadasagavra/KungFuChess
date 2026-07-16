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

    RenderConfig config_;
};

}  // namespace kfc::view
