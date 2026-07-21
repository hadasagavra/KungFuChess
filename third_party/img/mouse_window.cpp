#include "mouse_window.hpp"

#include <stdexcept>
#include <utility>

#include <opencv2/highgui.hpp>

#include "img.hpp"

MouseWindow::MouseWindow(std::string title) : title_(std::move(title)) {
    // Resizable window that preserves the image aspect ratio when the user
    // stretches it -- letterboxing (not distortion) fills the extra space.
    cv::namedWindow(title_, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::setMouseCallback(title_, &MouseWindow::onMouse, this);
}

MouseWindow::~MouseWindow() {
    // Destructors must not let exceptions escape (that would call
    // std::terminate), and closing a window the user already closed is a
    // perfectly normal way to get here -- so failures to close are swallowed.
    try {
        cv::destroyWindow(title_);
    } catch (const cv::Exception&) {
    }
}


void MouseWindow::showFrame(const Img& frame) {
    if (!frame.is_loaded()) {
        throw std::runtime_error("Frame not loaded.");
    }

    cv::imshow(title_, frame.get_mat());
    // Non-blocking: pump the GUI event queue (mouse callback, resize) for ~1ms
    // and return, so the caller keeps control of the render loop.
    cv::waitKey(1);
}

bool MouseWindow::isWindowOpen() const {
    // Drops below 1 once the user closes the window via its X button.
    return cv::getWindowProperty(title_, cv::WND_PROP_VISIBLE) >= 1;
}

std::vector<MouseEvent> MouseWindow::takeMouseEvents() {
    std::vector<MouseEvent> taken;
    taken.swap(events_);
    return taken;
}

void MouseWindow::onMouse(int event, int x, int y, int /*flags*/,
                          void* userdata) {
    // OpenCV already reports the position in image pixel coordinates: it maps the
    // window position through the current resize/aspect-ratio itself. So we store
    // (x, y) as-is -- doing our own scaling here would double-transform it.
    MouseWindow* self = static_cast<MouseWindow*>(userdata);
    if (event == cv::EVENT_LBUTTONDOWN) {
        self->events_.push_back({MouseEvent::Type::Click, x, y});
    } else if (event == cv::EVENT_LBUTTONDBLCLK) {
        // On Windows this is OpenCV's translation of WM_LBUTTONDBLCLK.
        self->events_.push_back({MouseEvent::Type::DoubleClick, x, y});
    }
}
