#include "img.hpp"
#include <iostream>
#include <stdexcept>

Img::Img() {
    // Constructor - img is automatically initialized as empty
}

Img& Img::read(const std::string& path,
               const std::pair<int, int>& size,
               bool keep_aspect,
               int interpolation) {
    img = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        throw std::runtime_error("Cannot load image: " + path);
    }

    if (size.first != 0 && size.second != 0) {  // Check if size is not empty
        int target_w = size.first;
        int target_h = size.second;
        int h = img.rows;
        int w = img.cols;

        if (keep_aspect) {
            double scale = std::min(static_cast<double>(target_w) / w, 
                                   static_cast<double>(target_h) / h);
            int new_w = static_cast<int>(w * scale);
            int new_h = static_cast<int>(h * scale);
            cv::resize(img, img, cv::Size(new_w, new_h), 0, 0, interpolation);
        } else {
            cv::resize(img, img, cv::Size(target_w, target_h), 0, 0, interpolation);
        }
    }

    return *this;
}

Img& Img::create(int width, int height, const cv::Scalar& color) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Canvas size must be positive.");
    }
    // 4-channel so that alpha-blended draws (highlights, cooldown shades) and
    // BGRA sprites composite onto it the same way they do onto a loaded board.
    img = cv::Mat(height, width, CV_8UC4, color);
    return *this;
}

void Img::draw_on(Img& other_img, int x, int y) {
    if (img.empty() || other_img.img.empty()) {
        throw std::runtime_error("Both images must be loaded before drawing.");
    }

    // Handle different channel counts.
    cv::Mat source_img = img;
    cv::Mat target_img = other_img.img;

    if (source_img.channels() == 3 && target_img.channels() == 4) {
        // Opaque source onto a BGRA target: give it a full-opacity alpha so the
        // blend below treats it as a solid copy.
        cv::cvtColor(source_img, source_img, cv::COLOR_BGR2BGRA);
    }
    // A BGRA source onto a BGR target is intentionally NOT flattened to BGR here:
    // doing so would discard the alpha and paste the (often black) transparent
    // background opaquely. Instead the source keeps its alpha and the blend below
    // composites it over the 3-channel destination.

    int h = source_img.rows;
    int w = source_img.cols;
    int H = target_img.rows;
    int W = target_img.cols;

    if (y + h > H || x + w > W) {
        throw std::runtime_error("Image does not fit at the specified position.");
    }

    cv::Mat roi = target_img(cv::Rect(x, y, w, h));

    if (source_img.channels() == 4) {
        // Alpha-blend a BGRA source over the destination region ("over"
        // compositing): dst = src*a + dst*(1-a), per colour channel.
        //
        // LOCAL FIX to the upstream Img port. The original code used
        //   roi.col(c) = (1.0 - alpha) * roi.col(c) + alpha * channels[c];
        // which (a) kept alpha as 8-bit ints, (b) addressed columns instead of
        // channels, and (c) used Mat*Mat (matrix multiply) -> crashed in
        // cv::gemm ("type == B.type()"). This does correct per-element blending.
        std::vector<cv::Mat> src_ch;
        cv::split(source_img, src_ch);  // B, G, R, A (8U)

        cv::Mat alpha;
        src_ch[3].convertTo(alpha, CV_32F, 1.0 / 255.0);  // a in [0, 1]
        cv::Mat inv_alpha = 1.0 - alpha;

        std::vector<cv::Mat> dst_ch;
        cv::split(roi, dst_ch);  // 3 or 4 channels, matching the target

        for (int c = 0; c < 3; ++c) {
            cv::Mat s, d;
            src_ch[c].convertTo(s, CV_32F);
            dst_ch[c].convertTo(d, CV_32F);
            cv::Mat blended = s.mul(alpha) + d.mul(inv_alpha);
            blended.convertTo(dst_ch[c], dst_ch[c].type());
        }
        if (static_cast<int>(dst_ch.size()) == 4) {
            // Preserve/accumulate the target alpha: outA = srcA + dstA*(1-srcA).
            cv::Mat dst_a;
            dst_ch[3].convertTo(dst_a, CV_32F, 1.0 / 255.0);
            cv::Mat out_a = alpha + dst_a.mul(inv_alpha);
            out_a.convertTo(dst_ch[3], dst_ch[3].type(), 255.0);
        }
        cv::Mat merged;
        cv::merge(dst_ch, merged);
        merged.copyTo(roi);
    } else {
        // Direct copy for BGR images
        source_img.copyTo(roi);
    }
}

void Img::put_text(const std::string& txt, int x, int y, double font_size,
                   const cv::Scalar& color, int thickness) {
    if (img.empty()) {
        throw std::runtime_error("Image not loaded.");
    }
    
    cv::putText(img, txt, cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, font_size,
                color, thickness, cv::LINE_AA);
}

void Img::draw_rect(int x, int y, int w, int h, const cv::Scalar& color,
                    int thickness, double alpha) {
    if (img.empty()) {
        throw std::runtime_error("Image not loaded.");
    }
    if (alpha >= 1.0) {
        // cv::rectangle clips to the image bounds on its own -- safe as-is.
        cv::rectangle(img, cv::Rect(x, y, w, h), color, thickness);
    } else {
        // The blend path builds a temporary ROI, so it would throw if the rect
        // spills off the image (e.g. a cell border at the board edge). Clamp the
        // ROI to the image, then draw the rectangle at its original position
        // relative to the clamped region so the visible part stays put -- and
        // cv::rectangle clips whatever still overhangs.
        const cv::Rect requested(x, y, w, h);
        const cv::Rect safe = requested & cv::Rect(0, 0, img.cols, img.rows);
        if (safe.width <= 0 || safe.height <= 0) {
            return;  // fully off-image: nothing to draw
        }
        cv::Mat roi = img(safe);
        cv::Mat overlay = roi.clone();
        cv::rectangle(overlay, cv::Rect(x - safe.x, y - safe.y, w, h), color,
                      thickness);
        cv::addWeighted(overlay, alpha, roi, 1.0 - alpha, 0, roi);
    }
}

void Img::show() {
    if (img.empty()) {
        throw std::runtime_error("Image not loaded.");
    }

    cv::imshow("Image", img);
    cv::waitKey(0);
    cv::destroyAllWindows();
}
