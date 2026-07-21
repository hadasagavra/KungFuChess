#include "third_party/doctest/doctest.h"

#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"

using kfc::view::computeLayout;
using kfc::view::defaultRenderConfig;
using kfc::view::FrameLayout;
using kfc::view::RenderConfig;
using kfc::view::visibleRowCapacity;

namespace {

constexpr int cellPx = 100;
constexpr int cells = 8;

RenderConfig config() { return defaultRenderConfig("assets", cellPx); }

}  // namespace

TEST_CASE("the frame is the board plus a panel on either side") {
    const RenderConfig cfg = config();
    const FrameLayout layout = computeLayout(cfg, cells, cells);

    CHECK(layout.frameWidth == cells * cellPx + 2 * cfg.panelWidthPx);
    CHECK(layout.frameHeight == cells * cellPx);
}

TEST_CASE("the board starts after the left panel") {
    const RenderConfig cfg = config();
    const FrameLayout layout = computeLayout(cfg, cells, cells);

    CHECK(layout.boardOrigin.x == cfg.panelWidthPx);
    CHECK(layout.boardOrigin.y == 0);
}

TEST_CASE("black sits on the left and white on the right") {
    const RenderConfig cfg = config();
    const FrameLayout layout = computeLayout(cfg, cells, cells);

    CHECK(layout.blackPanel.origin.x == 0);
    CHECK(layout.whitePanel.origin.x == cfg.panelWidthPx + cells * cellPx);
    CHECK(layout.blackPanel.width == cfg.panelWidthPx);
    CHECK(layout.whitePanel.width == cfg.panelWidthPx);
}

TEST_CASE("neither panel overlaps the board") {
    const RenderConfig cfg = config();
    const FrameLayout layout = computeLayout(cfg, cells, cells);

    const int boardLeft = layout.boardOrigin.x;
    const int boardRight = boardLeft + cells * cellPx;
    CHECK(layout.blackPanel.origin.x + layout.blackPanel.width <= boardLeft);
    CHECK(layout.whitePanel.origin.x >= boardRight);
    // And the right panel ends exactly at the frame's edge.
    CHECK(layout.whitePanel.origin.x + layout.whitePanel.width ==
          layout.frameWidth);
}

TEST_CASE("a non-square board is laid out from its own dimensions") {
    const RenderConfig cfg = config();
    const FrameLayout layout = computeLayout(cfg, 4, 6);

    CHECK(layout.frameWidth == 4 * cellPx + 2 * cfg.panelWidthPx);
    CHECK(layout.frameHeight == 6 * cellPx);
    CHECK(layout.blackPanel.height == 6 * cellPx);
}

TEST_CASE("row capacity is what fits below the panel header") {
    const RenderConfig cfg = config();
    const FrameLayout layout = computeLayout(cfg, cells, cells);

    const int capacity = visibleRowCapacity(cfg, layout.whitePanel);
    CHECK(capacity > 0);
    // Every row must fit inside the panel, header and padding included.
    CHECK(capacity * cfg.rowHeightPx + cfg.panelHeaderPx + cfg.panelPaddingPx <=
          layout.whitePanel.height);
}

TEST_CASE("a panel too short for even one row reports no capacity") {
    RenderConfig cfg = config();
    cfg.panelHeaderPx = 1000;  // header alone overruns the panel
    const FrameLayout layout = computeLayout(cfg, cells, cells);

    CHECK(visibleRowCapacity(cfg, layout.whitePanel) == 0);
}
