#include "third_party/doctest/doctest.h"

#include "client/view/include/lobby_view.hpp"

using kfc::view::buttonAt;
using kfc::view::LobbyAction;
using kfc::view::LobbyButton;
using kfc::view::lobbyButtons;
using kfc::view::PixelPoint;

namespace {

// A point in the middle of a button, where a click clearly lands on it.
PixelPoint center(const LobbyButton& button) {
    return PixelPoint{button.topLeft.x + button.width / 2,
                      button.topLeft.y + button.height / 2};
}

}  // namespace

TEST_CASE("the home screen offers Play and Room") {
    const std::vector<LobbyButton> buttons = lobbyButtons(false);
    REQUIRE(buttons.size() == 2);
    CHECK(buttonAt(buttons, center(buttons[0])) == LobbyAction::Play);
    CHECK(buttonAt(buttons, center(buttons[1])) == LobbyAction::OpenRoomDialog);
}

TEST_CASE("the room dialog offers Create, Join and Cancel") {
    const std::vector<LobbyButton> buttons = lobbyButtons(true);
    REQUIRE(buttons.size() == 3);
    CHECK(buttonAt(buttons, center(buttons[0])) == LobbyAction::Create);
    CHECK(buttonAt(buttons, center(buttons[1])) == LobbyAction::Join);
    CHECK(buttonAt(buttons, center(buttons[2])) == LobbyAction::Cancel);
}

TEST_CASE("a click on no button is None") {
    const std::vector<LobbyButton> buttons = lobbyButtons(false);
    CHECK(buttonAt(buttons, PixelPoint{2, 2}) == LobbyAction::None);
}
