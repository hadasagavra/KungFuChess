#pragma once

#include <optional>
#include <string>

#include "img.hpp"

// A mouse click reported by MouseWindow, already converted to IMAGE pixel
// coordinates (top-left origin). MouseWindow speaks pixels only -- it knows
// nothing about cells, board positions, or game rules; mapping a pixel to a cell
// happens above it, in the input layer.
struct ClickPos {
    int x;
    int y;
};

// An interactive, persistent on-screen window that displays frames and captures
// mouse clicks. This is the windowing/input half of the OpenCV boundary: like
// Img (a pure bitmap wrapper), MouseWindow is one of the ONLY places allowed to
// touch raw OpenCV -- here, the window + mouse-callback side of it. It reports
// clicks as raw pixels and holds no game rules and no board knowledge.
class MouseWindow {
public:
    /**
     * Open a persistent, resizable window and start listening for mouse clicks.
     * The window keeps the image aspect ratio when resized. Left-clicks and left
     * double-clicks are captured (in image pixels) via pollClick() /
     * pollDoubleClick().
     *
     * @param title Window title / OpenCV window name
     */
    void openWindow(const std::string& title = "KungFuChess");

    /**
     * Draw one frame into the window without blocking. Pumps the GUI event queue
     * (so clicks and resizes are processed) and returns immediately. Must be
     * called after openWindow().
     *
     * @param frame The image to display this tick
     */
    void showFrame(const Img& frame);

    /**
     * @return true while the window is open (false once the user closes it via
     *         the window's X button)
     */
    bool isWindowOpen() const;

    /**
     * Retrieve the last unread left-click, in image pixel coordinates, and clear
     * it. Returns std::nullopt if no new click occurred since the last call.
     */
    std::optional<ClickPos> pollClick();

    /**
     * Retrieve the last unread left double-click, in image pixel coordinates, and
     * clear it. Returns std::nullopt if no new double-click occurred since the
     * last call. On Windows this is OpenCV's translation of the native
     * WM_LBUTTONDBLCLK message.
     */
    std::optional<ClickPos> pollDoubleClick();

    /**
     * Close the window opened by openWindow().
     */
    void closeWindow();

private:
    // Mouse callback registered with OpenCV. userdata points at the owning
    // MouseWindow. Routes left-button presses to pendingClick_ and left
    // double-clicks to pendingDoubleClick_ (both in the image pixel coordinates
    // OpenCV reports).
    static void onMouse(int event, int x, int y, int flags, void* userdata);

    // windowTitle_ is empty when no window is open.
    std::string windowTitle_;
    std::optional<ClickPos> pendingClick_;        // last unread left-click (image px)
    std::optional<ClickPos> pendingDoubleClick_;  // last unread left double-click
};
