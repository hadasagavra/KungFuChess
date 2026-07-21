#pragma once

#include "model/include/game_event.hpp"

namespace kfc::engine {

// The Observer side of the pattern: anything that wants to hear about the game
// as it happens implements this. It lives beside the GameEngine because the
// engine is the Subject that raises these events -- the contract belongs with
// whoever publishes it, not with the domain data it carries.
//
// Both callbacks default to doing nothing, so an observer overrides only the
// events it actually cares about (a moves log ignores captures, a scoreboard
// ignores moves) instead of being forced to carry empty method bodies.
//
// Observers receive events; they never call back into the engine. That keeps the
// notification one-way and means a new listener -- a network publisher, say --
// can be added without the engine changing at all.
class GameObserver {
public:
    virtual ~GameObserver() = default;

    virtual void onMove(const model::MoveEvent& event) { (void)event; }
    virtual void onCapture(const model::CapturedPiece& captured) {
        (void)captured;
    }
};

}  // namespace kfc::engine
