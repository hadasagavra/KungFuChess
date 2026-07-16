#include "view/include/renderer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "view/include/asset_paths.hpp"

namespace kfc::view {

namespace {

// The cooldown indicator: a translucent black shade over the resting cell.
const cv::Scalar cooldownShadeColor{0, 0, 0, 255};  // BGRA; A kept opaque
const double cooldownShadeAlpha = 0.5;              // blend weight in [0, 1]

// The legal-move hint: a translucent yellow fill over a reachable cell.
const cv::Scalar highlightColor{0, 255, 255, 255};  // BGRA yellow; A kept opaque
const double highlightAlpha = 0.35;                 // blend weight in [0, 1]

}  // namespace

Renderer::Renderer(RenderConfig config) : config_(std::move(config)) {}

Img Renderer::renderFrame(const GameSnapshot& snapshot) const {
    Img frame = loadBoardBackground(snapshot.boardWidth, snapshot.boardHeight);
    drawHighlightsOn(frame, snapshot);
    for (const PieceView& piece : snapshot.pieces) {
        drawPieceOn(frame, piece);
        drawCooldownOn(frame, piece);
    }
    return frame;
}

Img Renderer::loadBoardBackground(int widthCells, int heightCells) const {
    Img board;
    board.read(config_.assetsRoot + "/board.png",
               {widthCells * config_.cellPx, heightCells * config_.cellPx});
    return board;
}

Img Renderer::loadSprite(model::Kind kind, model::Color color,
                         model::State state, int frame) const {
    Img sprite;
    sprite.read(spriteFramePath(config_.assetsRoot, kind, color, state, frame),
                {config_.cellPx, config_.cellPx});
    return sprite;
}

void Renderer::drawPieceOn(Img& frame, const PieceView& piece) const {
    Img sprite = loadSprite(piece.kind, piece.color, piece.state, piece.frame);
    sprite.draw_on(frame, piece.position.x, piece.position.y);
}

void Renderer::drawHighlightsOn(Img& frame, const GameSnapshot& snapshot) const {
    for (const PixelPoint& cell : snapshot.highlights) {
        frame.draw_rect(cell.x, cell.y, config_.cellPx, config_.cellPx,
                        highlightColor, cv::FILLED, highlightAlpha);
    }
}

void Renderer::drawCooldownOn(Img& frame, const PieceView& piece) const {
    if (piece.state != model::State::Resting) {
        return;
    }
    // The shade covers the remaining (not-yet-elapsed) fraction, anchored at the
    // bottom of the cell. As the rest completes its top edge descends toward the
    // bottom until the shade vanishes.
    const double remaining = std::clamp(1.0 - piece.restProgress, 0.0, 1.0);
    const int shadeHeight =
        static_cast<int>(std::lround(config_.cellPx * remaining));
    if (shadeHeight <= 0) {
        return;
    }
    const int shadeY = piece.position.y + (config_.cellPx - shadeHeight);
    frame.draw_rect(piece.position.x, shadeY, config_.cellPx, shadeHeight,
                    cooldownShadeColor, cv::FILLED, cooldownShadeAlpha);
}

}  // namespace kfc::view
