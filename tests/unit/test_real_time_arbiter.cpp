#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>

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
    CHECK_FALSE(reports[0].kingCaptured);
    CHECK(board.getPieceAt(Position{4, 5}).get() == rook.get());
    CHECK(board.getPieceAt(Position{4, 4}).get() == nullptr);
    CHECK(rook->getState() == State::Resting);  // cooldown, not idle
    CHECK_FALSE(arbiter.hasActiveMotion());
}

TEST_CASE("Motion reports its travel progress, clamped to [0,1]") {
    Motion motion{Position{4, 4}, Position{4, 6}};  // 2 cells = 2000ms
    CHECK(motion.progress() == doctest::Approx(0.0));
    motion.advance(500);
    CHECK(motion.progress() == doctest::Approx(0.25));
    motion.advance(1500);
    CHECK(motion.progress() == doctest::Approx(1.0));
    motion.advance(1000);  // past arrival stays clamped
    CHECK(motion.progress() == doctest::Approx(1.0));
}

TEST_CASE("activeMotions exposes each in-flight slide's from/to/progress") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.activeMotions().empty());

    arbiter.startMotion(Position{4, 4}, Position{0, 4});  // 4 cells = 4000ms
    arbiter.advance(1000);                                // quarter of the way

    const std::vector<MotionState> motions = arbiter.activeMotions();
    REQUIRE(motions.size() == 1);
    CHECK(motions[0].from == Position{4, 4});
    CHECK(motions[0].to == Position{0, 4});
    CHECK(motions[0].progress == doctest::Approx(0.25));

    arbiter.advance(3000);  // arrive
    CHECK(arbiter.activeMotions().empty());
}

TEST_CASE("Only one motion may be active at a time") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{0, 0});
    place(board, 2, Color::White, Kind::Rook, Position{7, 7});
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{0, 0}, Position{0, 1}));
    CHECK_FALSE(arbiter.startMotion(Position{7, 7}, Position{7, 6}));
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

TEST_CASE("Arrival captures an enemy and flags a king capture") {
    SUBCASE("capturing a pawn does not flag a king") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        auto enemy = place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{4, 4}, Position{4, 6});
        const std::vector<ArrivalReport> reports = arbiter.advance(2000);
        REQUIRE(reports.size() == 1);
        CHECK_FALSE(reports[0].kingCaptured);
        CHECK(enemy->getState() == State::Captured);
    }
    SUBCASE("capturing a king flags it") {
        Board board{8, 8};
        place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::Black, Kind::King, Position{4, 6});
        RealTimeArbiter arbiter{board};
        arbiter.startMotion(Position{4, 4}, Position{4, 6});
        const std::vector<ArrivalReport> reports = arbiter.advance(2000);
        REQUIRE(reports.size() == 1);
        CHECK(reports[0].kingCaptured);
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
