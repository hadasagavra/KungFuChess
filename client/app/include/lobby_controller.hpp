#pragma once

#include <string>

#include "client/net/include/lobby_access.hpp"
#include "client/view/include/lobby_view.hpp"

namespace kfc::app {

// The Home screen's controller: it holds the local UI state (whether the room
// dialog is open, and the room id being typed) and turns raw clicks and keys into
// lobby actions. It is the sibling of input::Controller for board play -- it
// drives a narrow net::LobbyAccess and knows nothing of sockets or drawing. The
// view() it produces is the drawable description the renderer paints.
class LobbyController {
public:
    explicit LobbyController(net::LobbyAccess& lobby);

    // A click at `point` (frame pixels): hit-test the buttons currently shown and
    // act -- Play/Create/Join/Cancel issue a lobby action, Room opens the dialog.
    void handleClick(view::PixelPoint point);

    // A typed key: edits the room-id box while the dialog is open; ignored
    // otherwise.
    void handleKey(int key);

    // The drawable Home screen for the current state.
    view::LobbyView view() const;

private:
    net::LobbyAccess& lobby_;
    bool dialogOpen_ = false;
    std::string roomText_;
};

}  // namespace kfc::app
