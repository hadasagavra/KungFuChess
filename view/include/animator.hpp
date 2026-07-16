#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "view/include/animation_config.hpp"
#include "view/include/game_snapshot.hpp"

namespace kfc::view {

// Which sprite frame (1-based) to show after playing an animation for elapsedMs.
// A looping animation cycles; a non-looping one holds on its last frame once it
// finishes. Display-only: it turns time into a frame number and encodes no game
// rule. Free + pure so it can be reasoned about and tested on its own.
inline int frameForElapsed(const AnimationConfig& config, int elapsedMs) {
    if (config.framesPerSec <= 0 || config.frameCount <= 0) {
        return firstSpriteFrame;
    }
    const int frameDurationMs = 1000 / config.framesPerSec;
    if (frameDurationMs <= 0) {
        return firstSpriteFrame;
    }
    const int step = elapsedMs / frameDurationMs;
    const int index = config.isLoop
                          ? step % config.frameCount
                          : std::min(step, config.frameCount - 1);
    return firstSpriteFrame + index;
}

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
    GameSnapshot animate(const GameSnapshot& snapshot, int deltaMs) {
        GameSnapshot animated = snapshot;
        std::unordered_set<std::uint32_t> present;
        for (PieceView& piece : animated.pieces) {
            present.insert(piece.id);
            const int elapsedMs = advanceTrack(piece, deltaMs);
            piece.frame =
                frameForElapsed(configFor_(piece.kind, piece.color, piece.state),
                                elapsedMs);
        }
        forgetAbsent(present);
        return animated;
    }

private:
    // One piece's playback position: which state is showing and for how long.
    struct Track {
        model::State state;
        int elapsedMs;
    };

    // Update the track for a piece and return its elapsed time. A newly seen
    // piece, or one whose state just changed, restarts at zero; otherwise time
    // accumulates.
    int advanceTrack(const PieceView& piece, int deltaMs) {
        auto found = tracks_.find(piece.id);
        if (found == tracks_.end()) {
            tracks_.emplace(piece.id, Track{piece.state, 0});
            return 0;
        }
        Track& track = found->second;
        if (track.state != piece.state) {
            track = Track{piece.state, 0};
        } else {
            track.elapsedMs += deltaMs;
        }
        return track.elapsedMs;
    }

    // Drop tracks for pieces no longer on the board (captured), so a later piece
    // reusing an id starts its animation fresh.
    void forgetAbsent(const std::unordered_set<std::uint32_t>& present) {
        for (auto it = tracks_.begin(); it != tracks_.end();) {
            it = present.count(it->first) ? std::next(it) : tracks_.erase(it);
        }
    }

    AnimationConfigProvider configFor_;
    std::unordered_map<std::uint32_t, Track> tracks_;
};

}  // namespace kfc::view
