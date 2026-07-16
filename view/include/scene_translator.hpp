#pragma once

#include <cmath>
#include <set>
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

// The active cooldown resting on a cell, or nullptr if none. A resting piece
// stays on its cell, so the seam matches the cooldown on that cell.
inline const realtime::CooldownState* cooldownAt(
    const std::vector<realtime::CooldownState>& cooldowns, model::Position cell) {
    for (const realtime::CooldownState& cooldown : cooldowns) {
        if (cooldown.cell == cell) {
            return &cooldown;
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
inline GameSnapshot buildSnapshot(const engine::GameSnapshot& state, int cellPx,
                                  const std::set<model::Position>& highlightCells = {}) {
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
