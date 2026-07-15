#pragma once

#include <string>

namespace kfc::view {

// Display configuration supplied by the composition root. cellPx is the single
// board-normalization factor (pixels per cell) shared by the seam (which turns
// cells into pixels) and the Renderer (which sizes the board and the sprites).
struct RenderConfig {
    std::string assetsRoot;
    int cellPx;
};

const int defaultCellPx = 100;

}  // namespace kfc::view
