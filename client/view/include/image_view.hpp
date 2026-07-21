#pragma once

#include <optional>
#include <vector>

#include "mouse_window.hpp"
#include "client/view/include/animation_config_store.hpp"
#include "client/view/include/animator.hpp"
#include "client/view/include/game_snapshot.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/renderer.hpp"

namespace kfc::view {

// A mouse action the view observed, in frame pixel coordinates. The view-level
// twin of the window's MouseEvent: it reports what happened and where in pixels,
// never a cell -- mapping a pixel to a board cell belongs to the input layer.
struct MouseAction {
    enum class Type { Click, DoubleClick };

    Type type;
    PixelPoint position;
};

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

    // The mouse actions observed since the last call, in the order the user
    // produced them, leaving none behind. Empty before open(). Display only:
    // reports pixels, never cells.
    std::vector<MouseAction> takeMouseActions();

private:
    Renderer renderer_;
    AnimationConfigStore configStore_;  // disk-backed animation configs
    Animator animator_;                 // display-side animation state machine
    // The window owns its OpenCV window for its whole lifetime, so it is created
    // by open() rather than with the view -- the one-shot show() path needs no
    // persistent window. Held in place: the mouse callback captures its address.
    std::optional<MouseWindow> window_;
};

}  // namespace kfc::view
