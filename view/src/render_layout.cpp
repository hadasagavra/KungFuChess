#include "view/include/render_layout.hpp"

#include <algorithm>

namespace kfc::view {

FrameLayout computeLayout(const RenderConfig& config, int boardCols,
                          int boardRows) {
    const int boardWidth = boardCols * config.cellPx;
    const int boardHeight = boardRows * config.cellPx;

    FrameLayout layout;
    layout.frameWidth = boardWidth + 2 * config.panelWidthPx;
    layout.frameHeight = boardHeight;
    layout.boardOrigin = PixelPoint{config.panelWidthPx, 0};

    layout.blackPanel =
        PanelBounds{PixelPoint{0, 0}, config.panelWidthPx, boardHeight};
    layout.whitePanel =
        PanelBounds{PixelPoint{config.panelWidthPx + boardWidth, 0},
                    config.panelWidthPx, boardHeight};
    return layout;
}

int visibleRowCapacity(const RenderConfig& config, const PanelBounds& panel) {
    // The header takes the top of the panel; the padding keeps the last row off
    // the bottom edge.
    const int tableHeight =
        panel.height - config.panelHeaderPx - config.panelPaddingPx;
    return std::max(0, tableHeight / config.rowHeightPx);
}

}  // namespace kfc::view
