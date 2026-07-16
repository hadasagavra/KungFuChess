#include "view/include/image_view.hpp"

#include <utility>

#include "img.hpp"

namespace kfc::view {

ImageView::ImageView(RenderConfig config)
    : renderer_(config),
      configStore_(config.assetsRoot),
      animator_([this](model::Kind kind, model::Color color,
                       model::State state) {
          return configStore_.configFor(kind, color, state);
      }) {}

void ImageView::show(const GameSnapshot& snapshot) const {
    Img frame = renderer_.renderFrame(snapshot);
    frame.show();
}

void ImageView::open() { window_.openWindow(); }

bool ImageView::isOpen() const { return window_.isWindowOpen(); }

void ImageView::render(const GameSnapshot& snapshot, int deltaMs) {
    Img frame = renderer_.renderFrame(animator_.animate(snapshot, deltaMs));
    window_.showFrame(frame);
}

std::optional<PixelPoint> ImageView::pollClick() {
    std::optional<ClickPos> click = window_.pollClick();
    if (!click) return std::nullopt;
    return PixelPoint{click->x, click->y};
}

}  // namespace kfc::view
