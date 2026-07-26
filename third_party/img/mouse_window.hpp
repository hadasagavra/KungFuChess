#pragma once

#include <string>
#include <vector>

// Img is only referred to by reference here, so a forward declaration is enough.
// This deliberately avoids including img.hpp (and through it all of OpenCV) from
// this header -- callers that just need the window API pay nothing for OpenCV.
class Img;

// Something the user did with the mouse, in IMAGE pixel coordinates (top-left
// origin). MouseWindow speaks pixels only -- it knows nothing about cells, board
// positions, or game rules; deciding what an event means happens above it, in
// the input layer.
struct MouseEvent {
    enum class Type { Click, DoubleClick };

    Type type;
    int x;
    int y;
};

// An interactive, persistent on-screen window that displays frames and captures
// mouse events. This is the windowing/input half of the OpenCV boundary: like
// Img (a pure bitmap wrapper), MouseWindow is one of the ONLY places allowed to
// touch raw OpenCV -- here, the window + mouse-callback side of it. It holds no
// game rules and no board knowledge.
//
// The window is owned: it opens on construction and closes on destruction. The
// mouse callback is registered with this object's address, so the object must
// stay put -- copying is disabled to keep that guarantee.
class MouseWindow {
public:
    /**
     * Open a persistent, resizable window and start listening for mouse events.
     * The window keeps the image aspect ratio when resized.
     *
     * @param title Window title / OpenCV window name
     */
    explicit MouseWindow(std::string title = "KungFuChess");
    ~MouseWindow();

    MouseWindow(const MouseWindow&) = delete;
    MouseWindow& operator=(const MouseWindow&) = delete;

    /**
     * Draw one frame into the window without blocking. Pumps the GUI event queue
     * (so mouse events and resizes are processed) and returns immediately.
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
     * Hand over the mouse events collected since the last call, leaving none
     * behind. Events keep the order the user produced them in, so a caller can
     * tell a lone click from the click that opens a double-click. Queuing rather
     * than dispatching keeps the window ignorant of what reacts to the events.
     */
    std::vector<MouseEvent> takeMouseEvents();

    /**
     * Hand over the key codes typed since the last call, leaving none behind.
     * Codes are raw (OpenCV waitKey): printable characters are their ASCII value;
     * Backspace is 8, Enter 13, Escape 27. Interpreting them is the caller's job.
     */
    std::vector<int> takeKeys();

private:
    // Mouse callback registered with OpenCV. userdata points at the owning
    // MouseWindow. Appends left clicks and left double-clicks to events_, in the
    // image pixel coordinates OpenCV reports. OpenCV runs this while the window
    // pumps its queue, which happens inside showFrame on this same thread, so
    // events_ needs no locking.
    static void onMouse(int event, int x, int y, int flags, void* userdata);

    std::string title_;
    std::vector<MouseEvent> events_;
    std::vector<int> keys_;
};
