#pragma once

#include <opencv2/opencv.hpp>
#include <optional>
#include <string>
#include <filesystem>

// A mouse click reported by Img, already converted to IMAGE pixel coordinates
// (top-left origin). Img speaks pixels only -- it knows nothing about cells,
// board positions, or game rules; mapping a pixel to a cell happens above Img.
struct ClickPos {
    int x;
    int y;
};

class Img {
public:
    Img();
    
    /**
     * Load image from path and optionally resize.
     * 
     * @param path Image file to load
     * @param size Target size in pixels (width, height). If empty, keep original
     * @param keep_aspect If true, shrink so the longer side fits size while preserving aspect ratio
     * @param interpolation OpenCV interpolation flag (e.g., cv::INTER_AREA for shrink, cv::INTER_LINEAR for enlarge)
     * @return Reference to this object for method chaining
     */
    Img& read(const std::string& path,
              const std::pair<int, int>& size = {},
              bool keep_aspect = false,
              int interpolation = cv::INTER_AREA);
    
    /**
     * Draw this image onto another image at position (x, y)
     * 
     * @param other_img The target image to draw on
     * @param x X coordinate for top-left corner
     * @param y Y coordinate for top-left corner
     */
    void draw_on(Img& other_img, int x, int y);
    
    /**
     * Put text on the image
     * 
     * @param txt Text to draw
     * @param x X coordinate for text position
     * @param y Y coordinate for text position (baseline)
     * @param font_size Font scale factor
     * @param color Text color (BGR or BGRA)
     * @param thickness Text thickness
     */
    void put_text(const std::string& txt, int x, int y, double font_size,
                  const cv::Scalar& color = cv::Scalar(255, 255, 255, 255),
                  int thickness = 1);
    
    /**
     * Display the image in a window. Blocks until a key is pressed, then closes
     * the window. Use the persistent-window methods below for an interactive,
     * non-blocking loop.
     */
    void show();

    /**
     * Open a persistent, resizable window and start listening for mouse clicks.
     * The window keeps the image aspect ratio when resized. Left-clicks are
     * captured and reported (in image pixels) via pollClick().
     *
     * @param title Window title / OpenCV window name
     */
    void openWindow(const std::string& title = "KungFuChess");

    /**
     * Draw one frame into the persistent window without blocking. Pumps the GUI
     * event queue (so clicks and resizes are processed) and returns immediately.
     * Must be called after openWindow().
     *
     * @param frame The image to display this tick
     */
    void showFrame(const Img& frame);

    /**
     * @return true while the persistent window is open (false once the user
     *         closes it via the window's X button)
     */
    bool isWindowOpen() const;

    /**
     * Retrieve the last unread left-click, in image pixel coordinates, and clear
     * it. Returns std::nullopt if no new click occurred since the last call.
     */
    std::optional<ClickPos> pollClick();

    /**
     * Close the persistent window opened by openWindow().
     */
    void closeWindow();

    /**
     * Get the underlying OpenCV Mat
     */
    const cv::Mat& get_mat() const { return img; }
    
    /**
     * Check if image is loaded
     */
    bool is_loaded() const { return !img.empty(); }

private:
    // Mouse callback registered with OpenCV. userdata points at the owning Img.
    // Filters to left-button presses and stores the click (which OpenCV already
    // reports in image pixel coordinates) in pendingClick_.
    static void onMouse(int event, int x, int y, int flags, void* userdata);

    cv::Mat img;

    // Persistent-window state. windowTitle_ is empty when no window is open.
    std::string windowTitle_;
    std::optional<ClickPos> pendingClick_;  // last unread left-click (image px)
};