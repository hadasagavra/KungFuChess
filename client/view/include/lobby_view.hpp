#pragma once

#include <string>
#include <vector>

#include "client/view/include/game_snapshot.hpp"  // PixelPoint

namespace kfc::view {

// What a click on the Home screen asks for. None means the click hit no button.
enum class LobbyAction {
    None,
    Play,            // find any opponent by rating
    OpenRoomDialog,  // show the Create/Join dialog
    Create,          // open a new room
    Join,            // join the room whose id is typed in the box
    Cancel           // close the dialog (or stop searching)
};

// One button on the Home screen: what it does, where it is (in lobby pixels), and
// what it reads.
struct LobbyButton {
    LobbyAction action;
    PixelPoint topLeft;
    int width;
    int height;
    std::string label;
};

// The whole Home screen as a drawable description: a title, a one-line status,
// the buttons to show, and -- when the room dialog is open -- the text a player
// has typed into the room-id box.
struct LobbyView {
    std::string title;
    std::string status;
    bool dialogOpen = false;
    std::string textbox;
    std::vector<LobbyButton> buttons;
};

// The fixed lobby frame size (the game frame has its own size).
constexpr int lobbyWidth = 700;
constexpr int lobbyHeight = 520;

// The buttons for the current mode (the two Home buttons, or the dialog's
// Create/Join/Cancel). One definition of where each sits, shared by the renderer
// that draws them and the hit-test that reads clicks.
std::vector<LobbyButton> lobbyButtons(bool dialogOpen);

// Which button, if any, a click at `point` fell on.
LobbyAction buttonAt(const std::vector<LobbyButton>& buttons, PixelPoint point);

}  // namespace kfc::view
