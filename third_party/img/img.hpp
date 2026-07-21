#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>

// A pure image/bitmap wrapper over OpenCV: load, composite, annotate, and hand
// back the underlying Mat. It draws pixels and nothing more -- no window, no
// input, no game rules. The interactive window and mouse capture live in the
// peer MouseWindow class (mouse_window.hpp).
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
     * Create a blank image of the given size, filled with a solid colour. Used
     * as a canvas when the frame is larger than any single loaded image -- e.g.
     * a board with side panels drawn around it.
     *
     * @param width Canvas width in pixels
     * @param height Canvas height in pixels
     * @param color Fill colour (BGR or BGRA)
     * @return Reference to this object for method chaining
     */
    Img& create(int width, int height, const cv::Scalar& color);
    
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
     * Draw a rectangle on the image, optionally alpha-blended over what is
     * already there. Used e.g. to overlay a rest/cooldown indicator on a cell.
     *
     * @param x X coordinate of the top-left corner
     * @param y Y coordinate of the top-left corner
     * @param w Rectangle width in pixels
     * @param h Rectangle height in pixels
     * @param color Rectangle color (BGR or BGRA)
     * @param thickness Border thickness; negative (cv::FILLED) fills the rect
     * @param alpha Opacity in [0, 1]; 1.0 draws opaque, below 1.0 blends
     */
    void draw_rect(int x, int y, int w, int h, const cv::Scalar& color,
                   int thickness = 1, double alpha = 1.0);
    
    /**
     * Display the image in a window. Blocks until a key is pressed, then closes
     * the window. Use the persistent-window methods below for an interactive,
     * non-blocking loop.
     */
    void show();

    /**
     * Get the underlying OpenCV Mat
     */
    const cv::Mat& get_mat() const { return img; }
    
    /**
     * Check if image is loaded
     */
    bool is_loaded() const { return !img.empty(); }

private:
    cv::Mat img;
};