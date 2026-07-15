#include "view/include/image_view.hpp"

#include <utility>

#include "img.hpp"

namespace kfc::view {

ImageView::ImageView(RenderConfig config) : renderer_(std::move(config)) {}

void ImageView::show(const GameSnapshot& snapshot) const {
    Img frame = renderer_.renderFrame(snapshot);
    frame.show();
}

void ImageView::open() { window_.openWindow(); }

bool ImageView::isOpen() const { return window_.isWindowOpen(); }

void ImageView::render(const GameSnapshot& snapshot) {
    Img frame = renderer_.renderFrame(snapshot);
    window_.showFrame(frame);
}

std::optional<PixelPoint> ImageView::pollClick() {
    std::optional<ClickPos> click = window_.pollClick();
    if (!click) return std::nullopt;
    return PixelPoint{click->x, click->y};
}

}  // namespace kfc::view
