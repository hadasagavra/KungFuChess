#include "mouse_window.hpp"

#include <stdexcept>

#include <opencv2/opencv.hpp>

void MouseWindow::openWindow(const std::string& title) {
    windowTitle_ = title;
    // Resizable window that preserves the image aspect ratio when the user
    // stretches it -- letterboxing (not distortion) fills the extra space.
    cv::namedWindow(windowTitle_, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::setMouseCallback(windowTitle_, &MouseWindow::onMouse, this);
}

void MouseWindow::showFrame(const Img& frame) {
    if (windowTitle_.empty()) {
        throw std::runtime_error("openWindow() must be called before showFrame().");
    }
    if (!frame.is_loaded()) {
        throw std::runtime_error("Frame not loaded.");
    }

    cv::imshow(windowTitle_, frame.get_mat());
    // Non-blocking: pump the GUI event queue (mouse callback, resize) for ~1ms
    // and return, so the caller keeps control of the render loop.
    cv::waitKey(1);
}

bool MouseWindow::isWindowOpen() const {
    if (windowTitle_.empty()) {
        return false;
    }
    // Drops below 1 once the user closes the window via its X button.
    return cv::getWindowProperty(windowTitle_, cv::WND_PROP_VISIBLE) >= 1;
}

std::optional<ClickPos> MouseWindow::pollClick() {
    std::optional<ClickPos> click = pendingClick_;
    pendingClick_.reset();
    return click;
}

std::optional<ClickPos> MouseWindow::pollDoubleClick() {
    std::optional<ClickPos> click = pendingDoubleClick_;
    pendingDoubleClick_.reset();
    return click;
}

void MouseWindow::closeWindow() {
    if (!windowTitle_.empty()) {
        cv::destroyWindow(windowTitle_);
        windowTitle_.clear();
    }
}

void MouseWindow::onMouse(int event, int x, int y, int /*flags*/, void* userdata) {
    // OpenCV already reports the click in image pixel coordinates: it maps the
    // window click through the current resize/aspect-ratio itself. So we store
    // (x, y) as-is -- doing our own scaling here would double-transform it.
    MouseWindow* self = static_cast<MouseWindow*>(userdata);
    if (event == cv::EVENT_LBUTTONDOWN) {
        self->pendingClick_ = ClickPos{x, y};
    } else if (event == cv::EVENT_LBUTTONDBLCLK) {
        // On Windows this is OpenCV's translation of WM_LBUTTONDBLCLK.
        self->pendingDoubleClick_ = ClickPos{x, y};
    }
}
