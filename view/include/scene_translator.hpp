#pragma once

#include <set>
#include <vector>

#include "engine/include/game_engine.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"
#include "view/include/game_snapshot.hpp"

namespace kfc::view {

// Top-left pixel of a board cell.
PixelPoint cellPixel(model::Position cell, int cellPx);

// Linear interpolation between two pixels at t in [0, 1], rounded to whole
// pixels. Used to place a sliding piece partway between its from/to cells.
PixelPoint lerpPixel(PixelPoint a, PixelPoint b, double t);

// The in-flight motion sliding out of a cell, or nullptr if none. A moving piece
// sits on its from cell in the logic, so the seam matches on that cell.
const realtime::MotionState* motionFrom(
    const std::vector<realtime::MotionState>& motions, model::Position cell);

// The active cooldown resting on a cell, or nullptr if none. A resting piece
// stays on its cell, so the seam matches the cooldown on that cell.
const realtime::CooldownState* cooldownAt(
    const std::vector<realtime::CooldownState>& cooldowns, model::Position cell);

// The GUI<-Logic output seam: projects the engine's live state onto a read-only
// display DTO. It is the graphical twin of io::printBoard (engine state -> text):
// here engine state -> pixels. It walks every cell and places each piece at
// pixel (col * cellPx, row * cellPx). It holds no game rules, mutates nothing,
// and never touches OpenCV -- so the Renderer keeps receiving only the pure
// view::GameSnapshot it is allowed to know, and this seam stays unit-testable.
GameSnapshot buildSnapshot(const engine::GameSnapshot& state, int cellPx,
                           const std::set<model::Position>& highlightCells = {});

}  // namespace kfc::view
