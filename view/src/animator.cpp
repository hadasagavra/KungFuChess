#include "view/include/animator.hpp"

#include <algorithm>
#include <iterator>

namespace kfc::view {

int frameForElapsed(const AnimationConfig& config, int elapsedMs) {
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

GameSnapshot Animator::animate(const GameSnapshot& snapshot, int deltaMs) {
    GameSnapshot animated = snapshot;
    std::unordered_set<std::uint32_t> present;
    for (PieceView& piece : animated.pieces) {
        present.insert(piece.id);
        const int elapsedMs = advanceTrack(piece, deltaMs);
        piece.frame = frameForElapsed(
            configFor_(piece.kind, piece.color, piece.state), elapsedMs);
    }
    forgetAbsent(present);
    return animated;
}

int Animator::advanceTrack(const PieceView& piece, int deltaMs) {
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

void Animator::forgetAbsent(const std::unordered_set<std::uint32_t>& present) {
    for (auto it = tracks_.begin(); it != tracks_.end();) {
        it = present.count(it->first) ? std::next(it) : tracks_.erase(it);
    }
}

}  // namespace kfc::view
