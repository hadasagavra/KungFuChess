#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <set>
#include <string>

#include "shared/logic/engine/include/game_engine.hpp"
#include "shared/logic/model/include/board.hpp"
#include "shared/logic/game_record/include/move_log.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/game_record/include/score_board.hpp"
#include "client/view/include/render_config.hpp"
#include "client/view/include/render_layout.hpp"
#include "client/view/include/scene_translator.hpp"

using kfc::engine::GameEngine;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::game_record::MoveLog;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::game_record::ScoreBoard;
using kfc::model::State;
using kfc::view::buildSnapshot;
using kfc::view::computeLayout;
using kfc::view::formatClock;
using kfc::view::FrameLayout;
using kfc::view::GameSnapshot;
using kfc::view::PieceView;
using kfc::view::RenderConfig;
using kfc::view::SceneInput;

namespace {

constexpr int cellPx = 100;
constexpr int boardCells = 8;

const RenderConfig& testConfig() {
    static const RenderConfig config =
        kfc::view::defaultRenderConfig("assets", cellPx);
    return config;
}

const FrameLayout& testLayout() {
    static const FrameLayout layout =
        computeLayout(testConfig(), boardCells, boardCells);
    return layout;
}

// The board is pushed right by Black's panel, so every board pixel in these
// expectations is measured from this origin rather than from zero.
int originX() { return testLayout().boardOrigin.x; }

void place(Board& board, std::uint32_t id, Color color, Kind kind,
           Position cell) {
    board.addPiece(std::make_shared<Piece>(id, color, kind, cell));
}

// The observers a scene needs. Held by the caller so the SceneInput's references
// stay valid for as long as it is used.
struct Listeners {
    MoveLog moveLog;
    ScoreBoard scoreBoard;
};

SceneInput sceneFor(const kfc::engine::GameSnapshot& state, Listeners& listeners,
                    std::set<Position> highlights = {}) {
    return SceneInput{state,      listeners.moveLog, listeners.scoreBoard,
                      std::move(highlights), "Alice", "Bob"};
}

}  // namespace

TEST_CASE("buildSnapshot carries the board dimensions") {
    Board board{8, 8};
    GameEngine engine{board};
    Listeners listeners;

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    CHECK(snapshot.boardWidth == 8);
    CHECK(snapshot.boardHeight == 8);
    CHECK(snapshot.pieces.empty());
}

TEST_CASE("buildSnapshot places a piece at its pixel position") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{1, 2});  // row 1, col 2
    GameEngine engine{board};
    Listeners listeners;

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    REQUIRE(snapshot.pieces.size() == 1);
    const PieceView& piece = snapshot.pieces.front();
    CHECK(piece.kind == Kind::Rook);
    CHECK(piece.color == Color::White);
    CHECK(piece.state == State::Idle);
    // x = boardOrigin.x + col * cellPx, y = row * cellPx.
    CHECK(piece.position.x == originX() + 200);
    CHECK(piece.position.y == 100);
}

TEST_CASE("buildSnapshot reports where the board starts in the frame") {
    Board board{8, 8};
    GameEngine engine{board};
    Listeners listeners;

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    CHECK(snapshot.boardOrigin == testLayout().boardOrigin);
}

TEST_CASE("buildSnapshot slides a moving piece between its cells") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{1, 2});
    GameEngine engine{board};
    Listeners listeners;

    REQUIRE(engine.requestMove(Position{1, 2}, Position{1, 3}).isAccepted);
    engine.wait(500);  // halfway through the 1000ms one-cell slide

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());
    REQUIRE(snapshot.pieces.size() == 1);
    const PieceView& piece = snapshot.pieces.front();
    CHECK(piece.state == State::Moving);
    // Halfway between col 2 and col 3; row 1 unchanged.
    CHECK(piece.position.x == originX() + 250);
    CHECK(piece.position.y == 100);
}

TEST_CASE("buildSnapshot maps highlight cells to their pixel corners") {
    Board board{8, 8};
    GameEngine engine{board};
    Listeners listeners;

    const std::set<Position> cells{Position{1, 2}, Position{3, 0}};
    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot = buildSnapshot(
        sceneFor(state, listeners, cells), testConfig(), testLayout());

    // std::set orders by (row, col), so {1,2} comes before {3,0}. Each highlight
    // is the top-left pixel of its cell, offset by the board's origin.
    REQUIRE(snapshot.highlights.size() == 2);
    CHECK(snapshot.highlights[0].x == originX() + 200);  // col 2
    CHECK(snapshot.highlights[0].y == 100);              // row 1
    CHECK(snapshot.highlights[1].x == originX() + 0);    // col 0
    CHECK(snapshot.highlights[1].y == 300);              // row 3
}

TEST_CASE("buildSnapshot leaves highlights empty by default") {
    Board board{8, 8};
    GameEngine engine{board};
    Listeners listeners;

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    CHECK(snapshot.highlights.empty());
}

TEST_CASE("buildSnapshot reports every occupied cell") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::King, Position{7, 4});
    place(board, 2, Color::Black, Kind::King, Position{0, 4});
    place(board, 3, Color::Black, Kind::Pawn, Position{1, 0});
    GameEngine engine{board};
    Listeners listeners;

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    CHECK(snapshot.pieces.size() == 3);
}

TEST_CASE("each panel carries its player's name") {
    Board board{8, 8};
    GameEngine engine{board};
    Listeners listeners;

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    CHECK(snapshot.whitePanel.name == "Alice");
    CHECK(snapshot.blackPanel.name == "Bob");
}

TEST_CASE("a move reaches the mover's panel as notation and a clock") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Knight, Position{7, 1});
    GameEngine engine{board};
    Listeners listeners;
    engine.events().subscribe<kfc::model::MoveEvent>(
        [&listeners](const kfc::model::MoveEvent& event) {
            listeners.moveLog.record(event);
        });

    engine.wait(2500);  // let the game clock run before the command
    REQUIRE(engine.requestMove(Position{7, 1}, Position{5, 2}).isAccepted);

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    REQUIRE(snapshot.whitePanel.moves.size() == 1);
    CHECK(snapshot.whitePanel.moves.front().move == "Nc3");
    CHECK(snapshot.whitePanel.moves.front().time == "00:02.500");
    // The other player's table is untouched.
    CHECK(snapshot.blackPanel.moves.empty());
}

TEST_CASE("a panel shows the player's score") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    place(board, 2, Color::Black, Kind::Queen, Position{0, 1});
    GameEngine engine{board};
    Listeners listeners;
    engine.events().subscribe<kfc::model::CapturedPiece>(
        [&listeners](const kfc::model::CapturedPiece& captured) {
            listeners.scoreBoard.record(captured);
        });

    REQUIRE(engine.requestMove(Position{0, 0}, Position{0, 1}).isAccepted);
    engine.wait(1000);  // complete the one-cell capture

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    CHECK(snapshot.whitePanel.score == 9);  // a queen
    CHECK(snapshot.blackPanel.score == 0);
}

TEST_CASE("a panel shows only as many recent moves as fit") {
    Board board{8, 8};
    GameEngine engine{board};
    Listeners listeners;

    const int capacity =
        kfc::view::visibleRowCapacity(testConfig(), testLayout().whitePanel);
    REQUIRE(capacity > 0);

    // Log more moves than the table has room for.
    for (int i = 0; i < capacity + 5; ++i) {
        listeners.moveLog.record(kfc::model::MoveEvent{
            i * 1000, Color::White, Kind::Pawn, Position{6, 0}, Position{5, 0},
            false, false});
    }

    const kfc::engine::GameSnapshot state = engine.getSnapshot();
    const GameSnapshot snapshot =
        buildSnapshot(sceneFor(state, listeners), testConfig(), testLayout());

    REQUIRE(snapshot.whitePanel.moves.size() == static_cast<size_t>(capacity));
    // The newest move survives; the oldest ones are the ones dropped.
    CHECK(snapshot.whitePanel.moves.back().time ==
          formatClock((capacity + 4) * 1000));
}

TEST_CASE("formatClock writes minutes, seconds and milliseconds") {
    CHECK(formatClock(0) == "00:00.000");
    CHECK(formatClock(4105) == "00:04.105");
    CHECK(formatClock(65432) == "01:05.432");
    CHECK(formatClock(600000) == "10:00.000");
}
