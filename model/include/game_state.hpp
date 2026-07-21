#pragma once

namespace kfc::model {

// The authoritative facts about the game as a whole: whether it has been decided,
// and how much game time has elapsed. It holds state and nothing else -- it never
// decides WHEN the game ends or how much time to add, only records what it is
// told.
//
// Elapsed time lives here, not in the display, because it is the timeline the
// game itself runs on: the moves log timestamps read from it, and a replay or a
// networked client would need the same number.
class GameState {
public:
    bool isOver() const { return over_; }
    void markOver() { over_ = true; }

    int elapsedMs() const { return elapsedMs_; }
    void advance(int ms) { elapsedMs_ += ms; }

private:
    bool over_ = false;
    int elapsedMs_ = 0;
};

}  // namespace kfc::model
