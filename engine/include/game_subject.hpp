#pragma once

#include <vector>

#include "engine/include/game_observer.hpp"
#include "model/include/game_event.hpp"

namespace kfc::engine {

// The Subject side of the pattern: it keeps the list of interested observers and
// fans events out to them. It holds no game data of its own -- knowing WHO is
// listening is a separate job from running the game, which is why this is not
// folded into the GameEngine.
//
// Observers are referenced, not owned: the composition root outlives them and
// decides their lifetime. An observer must therefore outlive the subject it
// registered with.
class GameSubject {
public:
    void addObserver(GameObserver& observer);

    void publishMove(const model::MoveEvent& event) const;
    void publishCapture(const model::CapturedPiece& captured) const;

private:
    std::vector<GameObserver*> observers_;
};

}  // namespace kfc::engine
