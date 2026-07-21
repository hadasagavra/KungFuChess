#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/motion.hpp"
#include "realtime/include/real_time_arbiter.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::model::State;
using kfc::realtime::ArrivalReport;
using kfc::realtime::Cooldown;
using kfc::realtime::CooldownState;
using kfc::realtime::Motion;
using kfc::realtime::MotionState;
using kfc::realtime::RealTimeArbiter;

namespace {

std::shared_ptr<Piece> place(Board& board, std::uint32_t id, Color color,
                             Kind kind, Position cell) {
    auto piece = std::make_shared<Piece>(id, color, kind, cell);
    board.addPiece(piece);
    return piece;
}

}  // namespace

TEST_CASE("A motion moves the piece only on arrival, then cools it down") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{4, 4}, Position{4, 5}));  // 1 cell = 1000ms
    CHECK(arbiter.hasActiveMotion());
    CHECK(rook->getState() == State::Moving);

    CHECK(arbiter.advance(999).empty());                        // sub-threshold
    CHECK(board.getPieceAt(Position{4, 4}).get() == rook.get());
    CHECK(board.getPieceAt(Position{4, 5}).get() == nullptr);

    const std::vector<ArrivalReport> reports = arbiter.advance(1);
    REQUIRE(reports.size() == 1);
    CHECK(reports[0].destination == Position{4, 5});
    CHECK_FALSE(reports[0].captured.has_value());
    CHECK(board.getPieceAt(Position{4, 5}).get() == rook.get());
    CHECK(board.getPieceAt(Position{4, 4}).get() == nullptr);
    CHECK(rook->getState() == State::Resting);  // cooldown, not idle
    CHECK_FALSE(arbiter.hasActiveMotion());
}

TEST_CASE("Motion reports progress through its current cell step") {
    Motion motion{Position{4, 4}, Position{4, 6}};  // 2 cells, 1000ms each
    CHECK(motion.stepProgress() == doctest::Approx(0.0));
    motion.advance(500);
    CHECK(motion.stepProgress() == doctest::Approx(0.5));
    motion.advance(500);
    CHECK(motion.stepProgress() == doctest::Approx(1.0));
    motion.advance(500);  // past the crossing stays clamped
    CHECK(motion.stepProgress() == doctest::Approx(1.0));
}

TEST_CASE("Motion walks its path one cell at a time, carrying leftover time") {
    Motion motion{Position{4, 4}, Position{4, 7}};  // 3 cells
    CHECK(motion.currentCell() == Position{4, 4});
    CHECK(motion.nextCell() == Position{4, 5});
    CHECK(motion.destination() == Position{4, 7});
    CHECK_FALSE(motion.isComplete());

    motion.advance(1500);  // past the first crossing, halfway into the second
    CHECK(motion.isEnteringNextCell());
    CHECK(motion.arrivalOvershootMs() == 500);

    motion.completeStep();  // the overshoot carries over, so timing never drifts
    CHECK(motion.currentCell() == Position{4, 5});
    CHECK(motion.stepProgress() == doctest::Approx(0.5));
    CHECK_FALSE(motion.isEnteringNextCell());

    motion.advance(1500);
    motion.completeStep();
    motion.completeStep();
    CHECK(motion.currentCell() == Position{4, 7});
    CHECK(motion.isComplete());
}

TEST_CASE("stopHere ends the journey on the cell the piece already holds") {
    Motion motion{Position{7, 4}, Position{0, 4}};
    motion.advance(1000);
    motion.completeStep();
    REQUIRE(motion.currentCell() == Position{6, 4});

    motion.stopHere();
    CHECK(motion.isComplete());
    CHECK(motion.destination() == Position{6, 4});
}

TEST_CASE("travelPath walks straight lines but leaps other shapes") {
    SUBCASE("a file is walked cell by cell") {
        const std::vector<Position> path =
            kfc::realtime::travelPath(Position{7, 4}, Position{4, 4});
        CHECK(path == std::vector<Position>{Position{7, 4}, Position{6, 4},
                                            Position{5, 4}, Position{4, 4}});
    }
    SUBCASE("a diagonal is walked cell by cell") {
        const std::vector<Position> path =
            kfc::realtime::travelPath(Position{0, 0}, Position{2, 2});
        CHECK(path == std::vector<Position>{Position{0, 0}, Position{1, 1},
                                            Position{2, 2}});
    }
    SUBCASE("a knight's leap touches nothing in between") {
        const std::vector<Position> path =
            kfc::realtime::travelPath(Position{4, 4}, Position{2, 5});
        CHECK(path == std::vector<Position>{Position{4, 4}, Position{2, 5}});
    }
}

TEST_CASE("activeMotions exposes the cell step each slide is in the middle of") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.activeMotions().empty());

    arbiter.startMotion(Position{4, 4}, Position{0, 4});  // 4 cells = 4000ms
    arbiter.advance(500);  // halfway between the first two cells

    const std::vector<MotionState> motions = arbiter.activeMotions();
    REQUIRE(motions.size() == 1);
    CHECK(motions[0].from == Position{4, 4});
    CHECK(motions[0].to == Position{3, 4});
    CHECK(motions[0].progress == doctest::Approx(0.5));

    arbiter.advance(1000);  // a cell on, and halfway through the next step
    const std::vector<MotionState> later = arbiter.activeMotions();
    REQUIRE(later.size() == 1);
    CHECK(later[0].from == Position{3, 4});
    CHECK(later[0].to == Position{2, 4});
    CHECK(later[0].progress == doctest::Approx(0.5));

    arbiter.advance(2500);  // arrive
    CHECK(arbiter.activeMotions().empty());
}

TEST_CASE("Motions of different pieces run side by side") {
    Board board{8, 8};
    auto first = place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    auto second = place(board, 2, Color::White, Kind::Rook, Position{7, 7});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{0, 0}, Position{0, 1}));
    CHECK(arbiter.startMotion(Position{7, 7}, Position{7, 6}));
    CHECK(arbiter.activeMotions().size() == 2);

    arbiter.advance(1000);  // both arrive together, neither in the other's way
    CHECK(board.getPieceAt(Position{0, 1}).get() == first.get());
    CHECK(board.getPieceAt(Position{7, 6}).get() == second.get());
}

TEST_CASE("A piece already travelling cannot start a second motion") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{0, 0}, Position{0, 4}));
    CHECK_FALSE(arbiter.startMotion(Position{0, 0}, Position{4, 0}));
    CHECK(arbiter.activeMotions().size() == 1);
}

TEST_CASE("A travelling piece occupies each cell of its path in turn") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{7, 4});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{7, 4}, Position{4, 4});  // 3 cells

    arbiter.advance(1000);
    CHECK(board.getPieceAt(Position{6, 4}).get() == rook.get());
    CHECK(rook->getState() == State::Moving);

    arbiter.advance(1000);
    CHECK(board.getPieceAt(Position{5, 4}).get() == rook.get());

    arbiter.advance(1000);
    CHECK(board.getPieceAt(Position{4, 4}).get() == rook.get());
    CHECK(rook->getState() == State::Resting);
}

TEST_CASE("A piece stops short of a friendly piece crossing its path") {
    // The rook runs up the e-file while its own queen crosses the fourth rank.
    // The queen holds e4 at the moment the rook would walk in, so the rook is
    // stuck on e3 and stays there.
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{7, 4});
    auto queen = place(board, 2, Color::White, Kind::Queen, Position{4, 0});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{4, 0}, Position{4, 7});  // queen, a4 -> h4
    arbiter.advance(1000);                                // queen reaches b4
    arbiter.startMotion(Position{7, 4}, Position{0, 4});  // rook, e1 -> e8

    arbiter.advance(3000);  // queen now holds e4; the rook is on e3, walking in

    CHECK(board.getPieceAt(Position{4, 4}).get() == queen.get());
    CHECK(board.getPieceAt(Position{5, 4}).get() == rook.get());
    CHECK(rook->getState() == State::Resting);  // stopped, not still travelling
    CHECK(arbiter.activeMotions().size() == 1);  // only the queen is left

    arbiter.advance(3000);  // the rook does not resume once the queen has passed
    CHECK(board.getPieceAt(Position{5, 4}).get() == rook.get());
}

TEST_CASE("An enemy met along the way is captured, not passed through") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 0});
    auto enemy = place(board, 2, Color::Black, Kind::Pawn, Position{4, 2});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{4, 0}, Position{4, 5});  // straight through it

    arbiter.advance(2000);  // reaches the pawn's cell
    CHECK(enemy->getState() == State::Captured);
    CHECK(board.getPieceAt(Position{4, 2}).get() == rook.get());
}

TEST_CASE("Of two enemies reaching a cell, the later arrival takes the earlier") {
    // Both rooks are bound for the same cell one step away, but the black rook
    // sets out 200ms after the white one, so it gets there second -- and wins.
    Board board{8, 8};
    auto white = place(board, 1, Color::White, Kind::Rook, Position{4, 3});
    auto black = place(board, 2, Color::Black, Kind::Rook, Position{4, 5});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{4, 3}, Position{4, 4});
    arbiter.advance(200);
    arbiter.startMotion(Position{4, 5}, Position{4, 4});

    arbiter.advance(1000);  // both crossings fall inside this one tick

    CHECK(board.getPieceAt(Position{4, 4}).get() == black.get());
    CHECK(white->getState() == State::Captured);
}

TEST_CASE("A piece caught mid-journey is captured and stops travelling") {
    // Both are under way. The white rook reaches {0,3} first and the black rook
    // walks into that same cell 500ms later, so the later arrival takes it -- and
    // the captured piece must not be carried on down its own path.
    Board board{8, 8};
    auto white = place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    auto black = place(board, 2, Color::Black, Kind::Rook, Position{1, 3});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{0, 0}, Position{0, 7});
    arbiter.advance(2500);  // white is on {0,2}, halfway towards {0,3}
    arbiter.startMotion(Position{1, 3}, Position{0, 3});

    // Within this tick white enters {0,3} and black follows it in 500ms later.
    arbiter.advance(1000);

    CHECK(white->getState() == State::Captured);
    CHECK(board.getPieceAt(Position{0, 3}).get() == black.get());
    CHECK(arbiter.activeMotions().empty());  // neither journey continues
}

TEST_CASE("Duration is the cell-step count (diagonal counts as steps)") {
    SUBCASE("three straight cells = 3000ms") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{4, 4}, Position{4, 7});
        CHECK(arbiter.advance(2999).empty());
        CHECK(arbiter.advance(1).size() == 1);
    }
    SUBCASE("three diagonal cells = 3000ms, not Euclidean") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Bishop, Position{0, 0});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{0, 0}, Position{3, 3});
        CHECK(arbiter.advance(2999).empty());
        CHECK(arbiter.advance(1).size() == 1);
    }
}

TEST_CASE("Cooldown reports its rest progress, clamped to [0,1]") {
    Cooldown cooldown{1, Position{4, 4}};  // 1000ms rest
    CHECK(cooldown.progress() == doctest::Approx(0.0));
    cooldown.advance(250);
    CHECK(cooldown.progress() == doctest::Approx(0.25));
    cooldown.advance(750);
    CHECK(cooldown.progress() == doctest::Approx(1.0));
    cooldown.advance(500);  // past the rest stays clamped
    CHECK(cooldown.progress() == doctest::Approx(1.0));
}

TEST_CASE("activeCooldowns exposes each resting piece's cell/progress") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.activeCooldowns().empty());

    arbiter.startMotion(Position{4, 4}, Position{4, 5});  // 1 cell = 1000ms
    arbiter.advance(1000);                                // arrives -> Resting
    arbiter.advance(250);                                 // quarter through rest

    const std::vector<CooldownState> cooldowns = arbiter.activeCooldowns();
    REQUIRE(cooldowns.size() == 1);
    CHECK(cooldowns[0].cell == Position{4, 5});
    CHECK(cooldowns[0].progress == doctest::Approx(0.25));

    arbiter.advance(750);  // rest elapses -> no longer resting
    CHECK(arbiter.activeCooldowns().empty());
}

TEST_CASE("Cooldown blocks the piece until it elapses, then it is idle again") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{4, 4}, Position{4, 5});
    arbiter.advance(1000);  // arrives -> Resting
    CHECK(rook->getState() == State::Resting);
    arbiter.advance(999);   // cooldown not yet elapsed
    CHECK(rook->getState() == State::Resting);
    arbiter.advance(1);     // cooldown elapsed
    CHECK(rook->getState() == State::Idle);
}

TEST_CASE("Arrival captures an enemy and names the victim") {
    SUBCASE("capturing a pawn reports that pawn") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        auto enemy = place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{4, 4}, Position{4, 6});
        const std::vector<ArrivalReport> reports = arbiter.advance(2000);
        REQUIRE(reports.size() == 1);
        REQUIRE(reports[0].captured.has_value());
        CHECK(reports[0].captured->kind == Kind::Pawn);
        CHECK(reports[0].captured->color == Color::Black);
        CHECK(enemy->getState() == State::Captured);
    }
    SUBCASE("capturing a king reports the king") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::Black, Kind::King, Position{4, 6});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{4, 4}, Position{4, 6});
        const std::vector<ArrivalReport> reports = arbiter.advance(2000);
        REQUIRE(reports.size() == 1);
        REQUIRE(reports[0].captured.has_value());
        CHECK(reports[0].captured->kind == Kind::King);
    }
    SUBCASE("an uncontested arrival reports no victim") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{4, 4}, Position{4, 6});
        const std::vector<ArrivalReport> reports = arbiter.advance(2000);
        REQUIRE(reports.size() == 1);
        CHECK_FALSE(reports[0].captured.has_value());
    }
}

TEST_CASE("A jump keeps the piece in place and cools it down on landing") {
    Board board{8, 8};
    auto knight = place(board, 1, Color::White, Kind::Knight, Position{4, 4});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startJump(Position{4, 4}));
    CHECK(knight->getState() == State::Airborne);

    CHECK(arbiter.advance(999).empty());  // still airborne, still in place
    CHECK(knight->getState() == State::Airborne);
    CHECK(board.getPieceAt(Position{4, 4}).get() == knight.get());

    const std::vector<ArrivalReport> reports = arbiter.advance(1);  // lands (1000ms)
    CHECK(reports.empty());  // a plain landing (no capture) produces no report
    CHECK(board.getPieceAt(Position{4, 4}).get() == knight.get());
    CHECK(knight->getState() == State::Resting);
    arbiter.advance(1000);
    CHECK(knight->getState() == State::Idle);
}

TEST_CASE("An airborne piece captures an enemy that arrives on its cell") {
    Board board{8, 8};
    auto jumper = place(board, 1, Color::White, Kind::Knight, Position{4, 4});
    auto enemy = place(board, 2, Color::Black, Kind::Rook, Position{4, 5});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startJump(Position{4, 4}));            // airborne for 1000ms
    CHECK(arbiter.startMotion(Position{4, 5}, Position{4, 4}));  // enemy, 1 cell = 1000ms

    arbiter.advance(1000);  // enemy arrives AND jump lands in the same tick

    // The airborne piece stayed and captured the arriving enemy.
    CHECK(board.getPieceAt(Position{4, 4}).get() == jumper.get());
    CHECK(enemy->getState() == State::Captured);
}
