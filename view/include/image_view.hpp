#pragma once

#include <optional>

#include "mouse_window.hpp"
#include "view/include/animation_config_store.hpp"
#include "view/include/animator.hpp"
#include "view/include/game_snapshot.hpp"
#include "view/include/render_config.hpp"
#include "view/include/renderer.hpp"

namespace kfc::view {

// The graphical view: composes a frame from a read-only GameSnapshot (via
// Renderer) and puts it on screen using only the Img wrapper. Display only --
// no game rules, no live domain objects, and no raw OpenCV.
class ImageView {
public:
    explicit ImageView(RenderConfig config);

    // Render the snapshot and display it. Blocks until a key is pressed, then
    // closes the window -- this is inherent to Img::show(). Kept for the simple
    // one-shot display path; use the persistent-window methods below for an
    // interactive loop.
    void show(const GameSnapshot& snapshot) const;

    // Open the persistent, resizable window used by the interactive loop.
    void open();

    // Whether the persistent window is still open (false once the user closes
    // it). Drives the caller's render loop.
    bool isOpen() const;

    // Render one frame of the snapshot into the persistent window, without
    // blocking. deltaMs is the wall-clock time since the previous render, which
    // advances the sprite animations. Call once per loop iteration after open().
    void render(const GameSnapshot& snapshot, int deltaMs);

    // The last unread click, in frame pixel coordinates, or std::nullopt if
    // none. Display only: returns a pixel, never a cell -- mapping a pixel to a
    // board cell belongs to the input layer, above the view.
    std::optional<PixelPoint> pollClick();

    // The last unread double-click, in frame pixel coordinates, or std::nullopt
    // if none. Same display-only contract as pollClick(): pixels, not cells.
    std::optional<PixelPoint> pollDoubleClick();

private:
    Renderer renderer_;
    AnimationConfigStore configStore_;  // disk-backed animation configs
    Animator animator_;                 // display-side animation state machine
    MouseWindow window_;  // owns the persistent window + raw click capture
};

}  // namespace kfc::view
