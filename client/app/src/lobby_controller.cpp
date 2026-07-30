#include "client/app/include/lobby_controller.hpp"

#include <cstddef>
#include <vector>

namespace kfc::app {
namespace {

constexpr const char* lobbyTitle = "KungFuChess";

// Apply one typed key to the room-id text box. Room ids are short uppercase
// alphanumerics, so letters are folded to upper case and other keys (bar
// Backspace) are ignored.
void editRoomText(std::string& text, int key) {
    constexpr std::size_t maxRoomIdLength = 8;
    if (key == 8 || key == 127) {  // Backspace / Delete
        if (!text.empty()) text.pop_back();
    } else if (key >= 'a' && key <= 'z') {
        if (text.size() < maxRoomIdLength) {
            text.push_back(static_cast<char>(key - 'a' + 'A'));
        }
    } else if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
        if (text.size() < maxRoomIdLength) text.push_back(static_cast<char>(key));
    }
}

}  // namespace

LobbyController::LobbyController(net::LobbyAccess& lobby) : lobby_(lobby) {}

void LobbyController::handleClick(view::PixelPoint point) {
    const std::vector<view::LobbyButton> buttons =
        view::lobbyButtons(dialogOpen_);
    switch (view::buttonAt(buttons, point)) {
        case view::LobbyAction::Play:
            lobby_.seekGame();
            break;
        case view::LobbyAction::OpenRoomDialog:
            dialogOpen_ = true;
            roomText_.clear();
            break;
        case view::LobbyAction::Create:
            lobby_.createRoom();
            dialogOpen_ = false;
            break;
        case view::LobbyAction::Join:
            if (!roomText_.empty()) lobby_.joinRoom(roomText_);
            dialogOpen_ = false;
            break;
        case view::LobbyAction::Cancel:
            if (lobby_.isSearching()) lobby_.cancelSeek();
            dialogOpen_ = false;
            break;
        case view::LobbyAction::None:
            break;
    }
}

void LobbyController::handleKey(int key) {
    if (dialogOpen_) editRoomText(roomText_, key);
}

view::LobbyView LobbyController::view() const {
    return view::LobbyView{lobbyTitle, lobby_.statusMessage(), dialogOpen_,
                           roomText_, view::lobbyButtons(dialogOpen_)};
}

}  // namespace kfc::app
