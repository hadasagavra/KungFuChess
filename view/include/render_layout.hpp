#pragma once

#include "view/include/game_snapshot.hpp"
#include "view/include/render_config.hpp"

namespace kfc::view {

// A rectangle in frame pixels: where a panel sits and how big it is.
struct PanelBounds {
    PixelPoint origin;
    int width;
    int height;
};

// Where everything goes in the frame. One arithmetic answer, computed once and
// then shared: the Renderer draws with it and the input mapper reads clicks with
// it, so what the player sees and what a click hits cannot drift apart.
//
// Layout is deliberately pure arithmetic with no drawing in it, which is what
// lets it be unit-tested without a graphics stack.
struct FrameLayout {
    int frameWidth;
    int frameHeight;
    PixelPoint boardOrigin;
    PanelBounds blackPanel;
    PanelBounds whitePanel;
};

// Black's panel on the left, the board in the middle, White's on the right --
// the arrangement in the product's reference screenshot.
FrameLayout computeLayout(const RenderConfig& config, int boardCols,
                          int boardRows);

// How many move rows fit in a panel's table area. The display shows the most
// recent ones when the history outgrows the space.
int visibleRowCapacity(const RenderConfig& config, const PanelBounds& panel);

}  // namespace kfc::view
