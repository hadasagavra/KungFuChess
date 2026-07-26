#include "client/view/include/image_view.hpp"

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

void ImageView::open() { window_.emplace(); }

bool ImageView::isOpen() const { return window_ && window_->isWindowOpen(); }

void ImageView::render(const GameSnapshot& snapshot, int deltaMs) {
    Img frame = renderer_.renderFrame(animator_.animate(snapshot, deltaMs));
    window_->showFrame(frame);
}

void ImageView::renderLobby(const LobbyView& view) {
    window_->showFrame(renderer_.renderLobby(view));
}

std::vector<int> ImageView::takeKeys() {
    if (!window_) {
        return {};
    }
    return window_->takeKeys();
}

std::vector<MouseAction> ImageView::takeMouseActions() {
    if (!window_) {
        return {};
    }
    std::vector<MouseAction> actions;
    for (const MouseEvent& event : window_->takeMouseEvents()) {
        actions.push_back(
            MouseAction{event.type == MouseEvent::Type::DoubleClick
                            ? MouseAction::Type::DoubleClick
                            : MouseAction::Type::Click,
                        PixelPoint{event.x, event.y}});
    }
    return actions;
}

}  // namespace kfc::view
