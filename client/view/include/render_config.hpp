#pragma once

#include <string>
#include <utility>

namespace kfc::view {

// Display configuration supplied by the composition root. cellPx is the single
// board-normalization factor (pixels per cell) shared by the seam (which turns
// cells into pixels), the Renderer (which sizes the board and the sprites), and
// the input mapper (which turns clicks back into cells).
//
// The panel fields size the two side tables that show each player's name, score,
// and move history. They live here so no pixel figure is written into the
// drawing code itself.
struct RenderConfig {
    std::string assetsRoot;
    int cellPx;
    int panelWidthPx;
    int panelPaddingPx;
    int rowHeightPx;
    int panelHeaderPx;
};

const int defaultCellPx = 100;
const int defaultPanelWidthPx = 220;
const int defaultPanelPaddingPx = 12;
const int defaultRowHeightPx = 22;
const int defaultPanelHeaderPx = 76;

// A RenderConfig using the default panel sizing, so callers that only care about
// the assets root and the cell size do not have to spell the rest out.
inline RenderConfig defaultRenderConfig(std::string assetsRoot, int cellPx) {
    return RenderConfig{std::move(assetsRoot),   cellPx,
                        defaultPanelWidthPx,     defaultPanelPaddingPx,
                        defaultRowHeightPx,      defaultPanelHeaderPx};
}

}  // namespace kfc::view
