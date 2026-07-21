#pragma once

#include <set>
#include <string>
#include <vector>

#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/game_record/include/move_log.hpp"
#include "shared/logic/game_record/include/score_board.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/realtime/include/motion.hpp"
#include "client/view/include/game_snapshot.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"

namespace kfc::view {

// Top-left pixel of a board cell, measured from the board's own origin within
// the frame.
PixelPoint cellPixel(model::Position cell, int cellPx, PixelPoint boardOrigin);

// Linear interpolation between two pixels at t in [0, 1], rounded to whole
// pixels. Used to place a sliding piece partway between its from/to cells.
PixelPoint lerpPixel(PixelPoint a, PixelPoint b, double t);

// Elapsed game time written for the moves table, e.g. 4105 -> "00:04.105".
// Formatting is a display concern, so the domain keeps plain milliseconds and
// the wording of a clock is decided here.
std::string formatClock(int ms);

// The in-flight motion sliding out of a cell, or nullptr if none. A moving piece
// sits on its from cell in the logic, so the seam matches on that cell.
const realtime::MotionState* motionFrom(
    const std::vector<realtime::MotionState>& motions, model::Position cell);

// The active cooldown resting on a cell, or nullptr if none. A resting piece
// stays on its cell, so the seam matches the cooldown on that cell.
const realtime::CooldownState* cooldownAt(
    const std::vector<realtime::CooldownState>& cooldowns, model::Position cell);

// Everything the seam needs to describe one frame. Gathered into one type
// because the composition root supplies it all together, and a growing argument
// list would say nothing about how the parts relate.
struct SceneInput {
    const engine::GameSnapshot& state;
    const game_record::MoveLog& moveLog;
    const game_record::ScoreBoard& scoreBoard;
    std::set<model::Position> highlights;
    std::string whiteName;
    std::string blackName;
};

// One player's side panel: their name, their score, and their moves written out
// as text. This is where domain history becomes display strings -- the notation
// comes from io, the clock from formatClock, and neither the log nor the
// Renderer has to know about the other.
PlayerPanel buildPanel(model::Color player, const SceneInput& input,
                       const RenderConfig& config, const PanelBounds& bounds);

// The GUI<-Logic output seam: projects the engine's live state onto a read-only
// display DTO. It is the graphical twin of io::printBoard (engine state -> text):
// here engine state -> pixels. It holds no game rules, mutates nothing, and
// never touches OpenCV -- so the Renderer keeps receiving only the pure
// view::GameSnapshot it is allowed to know, and this seam stays unit-testable.
GameSnapshot buildSnapshot(const SceneInput& input, const RenderConfig& config,
                           const FrameLayout& layout);

}  // namespace kfc::view
