#pragma once

#include <cmath>
#include <vector>

#include "engine/include/game_engine.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"
#include "view/include/game_snapshot.hpp"

namespace kfc::view {

// Top-left pixel of a board cell.
inline PixelPoint cellPixel(model::Position cell, int cellPx) {
    return PixelPoint{cell.col * cellPx, cell.row * cellPx};
}

// Linear interpolation between two pixels at t in [0, 1], rounded to whole
// pixels. Used to place a sliding piece partway between its from/to cells.
inline PixelPoint lerpPixel(PixelPoint a, PixelPoint b, double t) {
    return PixelPoint{
        a.x + static_cast<int>(std::lround((b.x - a.x) * t)),
        a.y + static_cast<int>(std::lround((b.y - a.y) * t))};
}

// The in-flight motion sliding out of a cell, or nullptr if none. A moving piece
// sits on its from cell in the logic, so the seam matches on that cell.
inline const realtime::MotionState* motionFrom(
    const std::vector<realtime::MotionState>& motions, model::Position cell) {
    for (const realtime::MotionState& motion : motions) {
        if (motion.from == cell) {
            return &motion;
        }
    }
    return nullptr;
}

// The GUI<-Logic output seam: projects the engine's live state onto a read-only
// display DTO. It is the graphical twin of io::printBoard (engine state -> text):
// here engine state -> pixels. It walks every cell and places each piece at
// pixel (col * cellPx, row * cellPx). It holds no game rules, mutates nothing,
// and never touches OpenCV -- so the Renderer keeps receiving only the pure
// view::GameSnapshot it is allowed to know, and this seam stays unit-testable.
//
// Header-only: it depends solely on two plain snapshot types, so both the app
// and the unit tests can use it without linking the OpenCV-backed view library.
inline GameSnapshot buildSnapshot(const engine::GameSnapshot& state, int cellPx) {
    GameSnapshot snapshot;
    snapshot.boardWidth = state.width();
    snapshot.boardHeight = state.height();

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

            snapshot.pieces.push_back(
                PieceView{piece->getKind(), piece->getColor(), piece->getState(),
                          pixel, piece->getId()});
        }
    }
    return snapshot;
}

}  // namespace kfc::view
