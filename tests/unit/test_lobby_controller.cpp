#include "third_party/doctest/doctest.h"

#include <string>
#include <vector>

#include "client/app/include/lobby_controller.hpp"
#include "client/net/include/lobby_access.hpp"
#include "client/view/include/lobby_view.hpp"

using kfc::app::LobbyController;
using kfc::net::LobbyAccess;
using kfc::view::LobbyAction;
using kfc::view::LobbyButton;
using kfc::view::lobbyButtons;
using kfc::view::PixelPoint;

namespace {

// A LobbyAccess that records what the controller asked of it, so a test can drive
// the controller with clicks/keys and read back the resulting actions.
class SpyLobby : public LobbyAccess {
public:
    int seeks = 0;
    int cancels = 0;
    int creates = 0;
    std::vector<std::string> joins;
    bool searching = false;
    std::string status;

    void seekGame() override {
        ++seeks;
        searching = true;
    }
    void cancelSeek() override {
        ++cancels;
        searching = false;
    }
    void createRoom() override { ++creates; }
    void joinRoom(const std::string& id) override { joins.push_back(id); }
    bool isSearching() const override { return searching; }
    std::string statusMessage() const override { return status; }
};

// A click at the centre of the button carrying `action` in the current mode.
PixelPoint clickOn(LobbyAction action, bool dialogOpen) {
    for (const LobbyButton& button : lobbyButtons(dialogOpen)) {
        if (button.action == action) {
            return PixelPoint{button.topLeft.x + button.width / 2,
                              button.topLeft.y + button.height / 2};
        }
    }
    return PixelPoint{-1, -1};
}

}  // namespace

TEST_CASE("Play seeks a game") {
    SpyLobby lobby;
    LobbyController controller{lobby};
    controller.handleClick(clickOn(LobbyAction::Play, false));
    CHECK(lobby.seeks == 1);
}

TEST_CASE("Room opens the dialog, and typing fills the box") {
    SpyLobby lobby;
    LobbyController controller{lobby};
    CHECK_FALSE(controller.view().dialogOpen);

    controller.handleClick(clickOn(LobbyAction::OpenRoomDialog, false));
    CHECK(controller.view().dialogOpen);

    controller.handleKey('a');  // folded to upper case
    controller.handleKey('7');
    CHECK(controller.view().textbox == "A7");
    controller.handleKey(8);  // Backspace
    CHECK(controller.view().textbox == "A");
}

TEST_CASE("keys are ignored while the dialog is closed") {
    SpyLobby lobby;
    LobbyController controller{lobby};
    controller.handleKey('A');
    CHECK(controller.view().textbox == "");
}

TEST_CASE("Create opens a room and closes the dialog") {
    SpyLobby lobby;
    LobbyController controller{lobby};
    controller.handleClick(clickOn(LobbyAction::OpenRoomDialog, false));
    controller.handleClick(clickOn(LobbyAction::Create, true));
    CHECK(lobby.creates == 1);
    CHECK_FALSE(controller.view().dialogOpen);
}

TEST_CASE("Join sends the typed room id, but does nothing when the box is empty") {
    SpyLobby lobby;
    LobbyController controller{lobby};

    // Empty box: Join is a no-op (it still closes the dialog).
    controller.handleClick(clickOn(LobbyAction::OpenRoomDialog, false));
    controller.handleClick(clickOn(LobbyAction::Join, true));
    CHECK(lobby.joins.empty());

    // Type an id, then join.
    controller.handleClick(clickOn(LobbyAction::OpenRoomDialog, false));
    controller.handleKey('A');
    controller.handleKey('B');
    controller.handleKey('1');
    controller.handleClick(clickOn(LobbyAction::Join, true));
    REQUIRE(lobby.joins.size() == 1);
    CHECK(lobby.joins[0] == "AB1");
}

TEST_CASE("Cancel while searching stops the search") {
    SpyLobby lobby;
    LobbyController controller{lobby};
    controller.handleClick(clickOn(LobbyAction::Play, false));  // now searching
    // The dialog can be opened while a search is outstanding.
    controller.handleClick(clickOn(LobbyAction::OpenRoomDialog, false));
    controller.handleClick(clickOn(LobbyAction::Cancel, true));
    CHECK(lobby.cancels == 1);
    CHECK_FALSE(controller.view().dialogOpen);
}

TEST_CASE("the view carries the lobby's status message") {
    SpyLobby lobby;
    lobby.status = "Searching...";
    LobbyController controller{lobby};
    CHECK(controller.view().status == "Searching...");
}
