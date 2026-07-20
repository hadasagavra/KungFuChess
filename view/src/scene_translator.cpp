#include "view/include/scene_translator.hpp"

#include <cmath>
#include <optional>

#include "model/include/piece.hpp"

namespace kfc::view {

PixelPoint cellPixel(model::Position cell, int cellPx) {
    return PixelPoint{cell.col * cellPx, cell.row * cellPx};
}

PixelPoint lerpPixel(PixelPoint a, PixelPoint b, double t) {
    return PixelPoint{
        a.x + static_cast<int>(std::lround((b.x - a.x) * t)),
        a.y + static_cast<int>(std::lround((b.y - a.y) * t))};
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

GameSnapshot buildSnapshot(const engine::GameSnapshot& state, int cellPx,
                           const std::set<model::Position>& highlightCells) {
    GameSnapshot snapshot;
    snapshot.boardWidth = state.width();
    snapshot.boardHeight = state.height();

    // Legal-move hints for the selected piece: the seam owns the cell->pixel
    // conversion here too, so the Renderer only ever paints pixels.
    for (const model::Position& cell : highlightCells) {
        snapshot.highlights.push_back(cellPixel(cell, cellPx));
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
            PixelPoint pixel = cellPixel(cell, cellPx);
            if (piece->getState() == model::State::Moving) {
                if (const realtime::MotionState* motion =
                        motionFrom(state.motions(), cell)) {
                    pixel = lerpPixel(cellPixel(motion->from, cellPx),
                                      cellPixel(motion->to, cellPx),
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
