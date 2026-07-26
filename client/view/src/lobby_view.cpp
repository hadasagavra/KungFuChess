#include "client/view/include/lobby_view.hpp"

namespace kfc::view {
namespace {

constexpr int buttonWidth = 220;
constexpr int buttonHeight = 64;
constexpr int centerX = lobbyWidth / 2;

LobbyButton centered(LobbyAction action, int y, const std::string& label,
                     int width = buttonWidth) {
    return LobbyButton{action, PixelPoint{centerX - width / 2, y}, width,
                       buttonHeight, label};
}

}  // namespace

std::vector<LobbyButton> lobbyButtons(bool dialogOpen) {
    if (!dialogOpen) {
        return {centered(LobbyAction::Play, 210, "Play"),
                centered(LobbyAction::OpenRoomDialog, 300, "Room")};
    }
    // The dialog: Create and Join side by side under the text box, Cancel below.
    constexpr int halfWidth = 100;
    constexpr int gap = 20;
    std::vector<LobbyButton> buttons;
    buttons.push_back(LobbyButton{LobbyAction::Create,
                                  PixelPoint{centerX - halfWidth - gap / 2, 300},
                                  halfWidth, buttonHeight, "Create"});
    buttons.push_back(LobbyButton{LobbyAction::Join,
                                  PixelPoint{centerX + gap / 2, 300}, halfWidth,
                                  buttonHeight, "Join"});
    buttons.push_back(centered(LobbyAction::Cancel, 390, "Cancel"));
    return buttons;
}

LobbyAction buttonAt(const std::vector<LobbyButton>& buttons, PixelPoint point) {
    for (const LobbyButton& button : buttons) {
        const bool inside = point.x >= button.topLeft.x &&
                            point.x < button.topLeft.x + button.width &&
                            point.y >= button.topLeft.y &&
                            point.y < button.topLeft.y + button.height;
        if (inside) return button.action;
    }
    return LobbyAction::None;
}

}  // namespace kfc::view
