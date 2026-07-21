#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "client/view/include/animation_config.hpp"
#include "client/view/include/game_snapshot.hpp"

namespace kfc::view {

// Which sprite frame (1-based) to show after playing an animation for elapsedMs.
// A looping animation cycles; a non-looping one holds on its last frame once it
// finishes. Display-only: it turns time into a frame number and encodes no game
// rule. Free + pure so it can be reasoned about and tested on its own.
int frameForElapsed(const AnimationConfig& config, int elapsedMs);

// The display-side animation state machine. Its "state" is the model::State the
// Business Logic reports for each piece; it owns none of the transition rules --
// the engine drives those, and the Animator only advances the sprite frame while
// a piece stays in a state. Per piece it tracks how long the current state has
// been showing; when the reported state changes it restarts that state's
// animation from its first frame. It holds no live domain object and never
// touches OpenCV, so it is unit-testable without the graphics stack.
class Animator {
public:
    explicit Animator(AnimationConfigProvider configFor)
        : configFor_(std::move(configFor)) {}

    // Advance every piece by deltaMs and return a copy of the snapshot with each
    // PieceView's frame set to the sprite to draw this tick.
    GameSnapshot animate(const GameSnapshot& snapshot, int deltaMs);

private:
    // One piece's playback position: which state is showing and for how long.
    struct Track {
        model::State state;
        int elapsedMs;
    };

    // Update the track for a piece and return its elapsed time. A newly seen
    // piece, or one whose state just changed, restarts at zero; otherwise time
    // accumulates.
    int advanceTrack(const PieceView& piece, int deltaMs);

    // Drop tracks for pieces no longer on the board (captured), so a later piece
    // reusing an id starts its animation fresh.
    void forgetAbsent(const std::unordered_set<std::uint32_t>& present);

    AnimationConfigProvider configFor_;
    std::unordered_map<std::uint32_t, Track> tracks_;
};

}  // namespace kfc::view
