#include "view/include/scene_translator.hpp"

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <optional>
#include <sstream>
#include <utility>

#include "io/include/move_notation.hpp"
#include "model/include/piece.hpp"

namespace kfc::view {
namespace {

constexpr int msPerSecond = 1000;
constexpr int secondsPerMinute = 60;
constexpr int minuteDigits = 2;
constexpr int secondDigits = 2;
constexpr int millisecondDigits = 3;

// Zero-padded field, so the columns of the moves table line up.
std::string padded(int value, int digits) {
    std::ostringstream out;
    out << std::setw(digits) << std::setfill('0') << value;
    return out.str();
}

}  // namespace

PixelPoint cellPixel(model::Position cell, int cellPx, PixelPoint boardOrigin) {
    return PixelPoint{boardOrigin.x + cell.col * cellPx,
                      boardOrigin.y + cell.row * cellPx};
}

PixelPoint lerpPixel(PixelPoint a, PixelPoint b, double t) {
    return PixelPoint{
        a.x + static_cast<int>(std::lround((b.x - a.x) * t)),
        a.y + static_cast<int>(std::lround((b.y - a.y) * t))};
}

std::string formatClock(int ms) {
    const int totalSeconds = ms / msPerSecond;
    return padded(totalSeconds / secondsPerMinute, minuteDigits) + ":" +
           padded(totalSeconds % secondsPerMinute, secondDigits) + "." +
           padded(ms % msPerSecond, millisecondDigits);
}

const realtime::MotionState* motionFrom(
    const std::vector<realtime::MotionState>& motions, model::Position cell) {
    for (const realtime::MotionState& motion : motions) {
        if (motion.from == cell) {
            return &motion;
        }
    }
    return nullptr;
}

const realtime::CooldownState* cooldownAt(
    const std::vector<realtime::CooldownState>& cooldowns,
    model::Position cell) {
    for (const realtime::CooldownState& cooldown : cooldowns) {
        if (cooldown.cell == cell) {
            return &cooldown;
        }
    }
    return nullptr;
}

PlayerPanel buildPanel(model::Color player, const SceneInput& input,
                       const RenderConfig& config, const PanelBounds& bounds) {
    PlayerPanel panel;
    panel.name = player == model::Color::White ? input.whiteName
                                               : input.blackName;
    panel.score = input.scoreBoard.scoreFor(player);

    const std::vector<model::MoveEvent>& entries =
        input.moveLog.entriesFor(player);

    // A long game outgrows the panel, so show the most recent moves -- the ones
    // a player is actually looking for -- rather than the opening.
    const std::size_t capacity =
        static_cast<std::size_t>(visibleRowCapacity(config, bounds));
    const std::size_t first =
        entries.size() > capacity ? entries.size() - capacity : 0;

    for (std::size_t i = first; i < entries.size(); ++i) {
        panel.moves.push_back(
            MoveRow{formatClock(entries[i].timeMs),
                    io::moveNotation(entries[i], input.state.height())});
    }
    return panel;
}

GameSnapshot buildSnapshot(const SceneInput& input, const RenderConfig& config,
                           const FrameLayout& layout) {
    const engine::GameSnapshot& state = input.state;
    const int cellPx = config.cellPx;

    GameSnapshot snapshot;
    snapshot.boardWidth = state.width();
    snapshot.boardHeight = state.height();
    snapshot.boardOrigin = layout.boardOrigin;
    snapshot.whitePanel =
        buildPanel(model::Color::White, input, config, layout.whitePanel);
    snapshot.blackPanel =
        buildPanel(model::Color::Black, input, config, layout.blackPanel);

    // Legal-move hints for the selected piece: the seam owns the cell->pixel
    // conversion here too, so the Renderer only ever paints pixels.
    for (const model::Position& cell : input.highlights) {
        snapshot.highlights.push_back(
            cellPixel(cell, cellPx, layout.boardOrigin));
    }

    for (int row = 0; row < state.height(); ++row) {
        for (int col = 0; col < state.width(); ++col) {
            const model::Position cell{row, col};
            const std::optional<model::Piece> piece = state.pieceAt(cell);
            if (!piece) {
                continue;
            }

            // A piece slides between cells while moving; otherwise it sits on its
            // cell. The seam owns this cell->pixel conversion, including transit.
            PixelPoint pixel = cellPixel(cell, cellPx, layout.boardOrigin);
            if (piece->getState() == model::State::Moving) {
                if (const realtime::MotionState* motion =
                        motionFrom(state.motions(), cell)) {
                    pixel = lerpPixel(
                        cellPixel(motion->from, cellPx, layout.boardOrigin),
                        cellPixel(motion->to, cellPx, layout.boardOrigin),
                        motion->progress);
                }
            }

            PieceView pieceView{piece->getKind(), piece->getColor(),
                                piece->getState(), pixel, piece->getId()};

            // A resting piece carries its cooldown progress so the view can draw
            // a shrinking overlay; other states leave it at 0.
            if (piece->getState() == model::State::Resting) {
                if (const realtime::CooldownState* cooldown =
                        cooldownAt(state.cooldowns(), cell)) {
                    pieceView.restProgress = cooldown->progress;
                }
            }

            snapshot.pieces.push_back(pieceView);
        }
    }
    return snapshot;
}

}  // namespace kfc::view
