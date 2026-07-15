#include "view/include/renderer.hpp"

#include <utility>

#include "view/include/asset_paths.hpp"

namespace kfc::view {
namespace {

// Sprite frame drawn for a static (non-animated) render.
const int staticFrame = 1;

}  // namespace

Renderer::Renderer(RenderConfig config) : config_(std::move(config)) {}

Img Renderer::renderFrame(const GameSnapshot& snapshot) const {
    Img frame = loadBoardBackground(snapshot.boardWidth, snapshot.boardHeight);
    for (const PieceView& piece : snapshot.pieces) {
        drawPieceOn(frame, piece);
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
                         model::State state) const {
    Img sprite;
    sprite.read(
        spriteFramePath(config_.assetsRoot, kind, color, state, staticFrame),
        {config_.cellPx, config_.cellPx});
    return sprite;
}

void Renderer::drawPieceOn(Img& frame, const PieceView& piece) const {
    Img sprite = loadSprite(piece.kind, piece.color, piece.state);
    sprite.draw_on(frame, piece.position.x, piece.position.y);
}

}  // namespace kfc::view
