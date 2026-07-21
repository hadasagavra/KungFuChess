#include "client/view/include/renderer.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "client/view/include/asset_paths.hpp"

namespace kfc::view {

namespace {

// The cooldown indicator: a translucent black shade over the resting cell.
const cv::Scalar cooldownShadeColor{0, 0, 0, 255};  // BGRA; A kept opaque
const double cooldownShadeAlpha = 0.5;              // blend weight in [0, 1]

// The legal-move hint: a translucent yellow fill over a reachable cell.
const cv::Scalar highlightColor{0, 255, 255, 255};  // BGRA yellow; A kept opaque
const double highlightAlpha = 0.35;                 // blend weight in [0, 1]

// The side panels: a grey surround with white cards for the tables.
const cv::Scalar frameBackColor{110, 110, 110, 255};
const cv::Scalar panelCardColor{255, 255, 255, 255};
const cv::Scalar panelTextColor{30, 30, 30, 255};
const cv::Scalar panelHeadingColor{90, 90, 90, 255};
const cv::Scalar panelRuleColor{200, 200, 200, 255};

const double nameFontSize = 0.62;
const double scoreFontSize = 0.58;
const double headingFontSize = 0.45;
const double rowFontSize = 0.42;
const int boldText = 2;
const int normalText = 1;

// Vertical rhythm inside a panel header, measured down from its top edge.
const int nameBaselineY = 30;
const int scoreBaselineY = 56;
// Where the "Time"/"Move" headings sit, and where the first move row starts.
const int headingBaselineOffset = -10;
const int rowTextBaselineOffset = -6;
// The move column's offset from the panel's left text edge; the time column
// starts at the edge itself.
const int moveColumnOffset = 96;

const char* const scoreLabel = "Score: ";
const char* const timeHeading = "Time";
const char* const moveHeading = "Move";

}  // namespace

Renderer::Renderer(RenderConfig config) : config_(std::move(config)) {}

Img Renderer::renderFrame(const GameSnapshot& snapshot) const {
    const FrameLayout layout =
        computeLayout(config_, snapshot.boardWidth, snapshot.boardHeight);

    Img frame = createCanvas(layout);
    drawBoardOn(frame, snapshot);
    drawHighlightsOn(frame, snapshot);
    for (const PieceView& piece : snapshot.pieces) {
        drawPieceOn(frame, piece);
        drawCooldownOn(frame, piece);
    }
    drawPanelOn(frame, snapshot.blackPanel, layout.blackPanel);
    drawPanelOn(frame, snapshot.whitePanel, layout.whitePanel);
    return frame;
}

Img Renderer::createCanvas(const FrameLayout& layout) const {
    Img canvas;
    canvas.create(layout.frameWidth, layout.frameHeight, frameBackColor);
    return canvas;
}

Img Renderer::loadBoardBackground(int widthCells, int heightCells) const {
    Img board;
    board.read(config_.assetsRoot + "/board.png",
               {widthCells * config_.cellPx, heightCells * config_.cellPx});
    return board;
}

void Renderer::drawBoardOn(Img& frame, const GameSnapshot& snapshot) const {
    Img board = loadBoardBackground(snapshot.boardWidth, snapshot.boardHeight);
    board.draw_on(frame, snapshot.boardOrigin.x, snapshot.boardOrigin.y);
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

void Renderer::drawPanelOn(Img& frame, const PlayerPanel& panel,
                           const PanelBounds& bounds) const {
    drawPanelHeaderOn(frame, panel, bounds);
    drawMoveTableOn(frame, panel, bounds);
}

void Renderer::drawPanelHeaderOn(Img& frame, const PlayerPanel& panel,
                                 const PanelBounds& bounds) const {
    const int pad = config_.panelPaddingPx;
    const int cardWidth = bounds.width - 2 * pad;
    const int textX = bounds.origin.x + pad;

    frame.draw_rect(textX, bounds.origin.y + pad, cardWidth,
                    config_.panelHeaderPx - pad, panelCardColor, cv::FILLED);
    frame.put_text(panel.name, textX + pad, bounds.origin.y + nameBaselineY,
                   nameFontSize, panelTextColor, boldText);
    frame.put_text(scoreLabel + std::to_string(panel.score), textX + pad,
                   bounds.origin.y + scoreBaselineY, scoreFontSize,
                   panelTextColor, normalText);
}

void Renderer::drawMoveTableOn(Img& frame, const PlayerPanel& panel,
                               const PanelBounds& bounds) const {
    const int pad = config_.panelPaddingPx;
    const int cardWidth = bounds.width - 2 * pad;
    const int textX = bounds.origin.x + pad;
    const int tableTop = bounds.origin.y + config_.panelHeaderPx;
    const int tableHeight = bounds.height - config_.panelHeaderPx - pad;
    if (tableHeight <= 0) {
        return;
    }

    frame.draw_rect(textX, tableTop, cardWidth, tableHeight, panelCardColor,
                    cv::FILLED);

    const int timeX = textX + pad;
    const int moveX = timeX + moveColumnOffset;
    const int headingBaseline =
        tableTop + config_.rowHeightPx + headingBaselineOffset;
    frame.put_text(timeHeading, timeX, headingBaseline, headingFontSize,
                   panelHeadingColor, normalText);
    frame.put_text(moveHeading, moveX, headingBaseline, headingFontSize,
                   panelHeadingColor, normalText);
    frame.draw_rect(timeX, headingBaseline + pad / 2, cardWidth - 2 * pad, 1,
                    panelRuleColor, cv::FILLED);

    // Rows run below the heading; the seam has already limited how many there
    // are to what fits, so nothing here can overflow the card.
    int rowIndex = 1;
    for (const MoveRow& row : panel.moves) {
        const int baseline = headingBaseline + rowIndex * config_.rowHeightPx +
                             rowTextBaselineOffset;
        if (baseline > tableTop + tableHeight) {
            break;
        }
        frame.put_text(row.time, timeX, baseline, rowFontSize, panelTextColor,
                       normalText);
        frame.put_text(row.move, moveX, baseline, rowFontSize, panelTextColor,
                       normalText);
        ++rowIndex;
    }
}

}  // namespace kfc::view
