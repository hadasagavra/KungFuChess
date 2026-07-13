#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <stdexcept>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"
#include "realtime/include/real_time_arbiter.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::model::State;
using kfc::realtime::ArbiterResult;
using kfc::realtime::RealTimeArbiter;

namespace {

std::shared_ptr<Piece> place(Board& board, std::uint32_t id, Color color,
                             Kind kind, Position cell) {
    auto piece = std::make_shared<Piece>(id, color, kind, cell);
    board.addPiece(piece);
    return piece;
}

}  // namespace

TEST_CASE("A straight one-cell move takes exactly 1000ms") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(1, Position{4, 4}, Position{4, 5});

    SUBCASE("just below threshold: nothing has moved") {
        arbiter.advanceTime(999);
        CHECK(arbiter.hasActiveMotion());
        CHECK(board.getPieceAt(Position{4, 4}).get() == rook.get());
        CHECK(board.getPieceAt(Position{4, 5}).get() == nullptr);
        CHECK(rook->getCell() == Position{4, 4});
    }
    SUBCASE("at threshold: the piece arrives") {
        arbiter.advanceTime(1000);
        CHECK_FALSE(arbiter.hasActiveMotion());
        CHECK(board.getPieceAt(Position{4, 4}).get() == nullptr);
        CHECK(board.getPieceAt(Position{4, 5}).get() == rook.get());
        CHECK(rook->getCell() == Position{4, 5});
    }
}

TEST_CASE("A straight N-cell move takes N*1000ms") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(1, Position{4, 4}, Position{4, 7});  // 3 cells

    arbiter.advanceTime(2999);
    CHECK(arbiter.hasActiveMotion());

    arbiter.advanceTime(1);  // reaches 3000
    CHECK_FALSE(arbiter.hasActiveMotion());
    CHECK(board.getPieceAt(Position{4, 7}).get() != nullptr);
}

TEST_CASE("A diagonal move uses cell-step count, not Euclidean distance") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Bishop, Position{1, 1});
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(1, Position{1, 1}, Position{4, 4});  // 3 diagonal cells

    arbiter.advanceTime(2999);
    CHECK(arbiter.hasActiveMotion());  // Euclidean (~4243ms) would still be far off too,
                                       // but the point is it is NOT yet arrived at 2999

    arbiter.advanceTime(1);  // exactly 3000ms -> arrives (proves duration == 3000)
    CHECK_FALSE(arbiter.hasActiveMotion());
    CHECK(board.getPieceAt(Position{4, 4}).get() != nullptr);
}

TEST_CASE("Cumulative sub-threshold advances never mutate the board") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(1, Position{4, 4}, Position{4, 5});  // 1000ms

    arbiter.advanceTime(500);
    arbiter.advanceTime(499);  // total 999 < 1000

    CHECK(arbiter.hasActiveMotion());
    CHECK(board.getPieceAt(Position{4, 4}).get() == rook.get());
    CHECK(board.getPieceAt(Position{4, 5}).get() == nullptr);
}

TEST_CASE("Arrival syncs the piece and updates its state to Idle") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(1, Position{4, 4}, Position{4, 5});
    CHECK(rook->getState() == State::Moving);  // set while in flight

    SUBCASE("exact arrival") {
        arbiter.advanceTime(1000);
        CHECK(rook->getCell() == Position{4, 5});
        CHECK(rook->getState() == State::Idle);
    }
    SUBCASE("overshoot still arrives cleanly") {
        arbiter.advanceTime(1500);  // past 1000
        CHECK_FALSE(arbiter.hasActiveMotion());
        CHECK(rook->getCell() == Position{4, 5});
        CHECK(rook->getState() == State::Idle);
    }
}

TEST_CASE("Capturing an enemy on arrival replaces it and marks it captured") {
    Board board{8, 8};
    auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    auto enemy = place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});  // 2 cells
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(1, Position{4, 4}, Position{4, 6});

    const ArbiterResult result = arbiter.advanceTime(2000);

    CHECK(board.getPieceAt(Position{4, 6}).get() == rook.get());
    CHECK(enemy->getState() == State::Captured);
    CHECK_FALSE(result.kingCaptured);  // pawn is not a king
}

TEST_CASE("Capturing a King is flagged exactly once") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    auto king = place(board, 2, Color::Black, Kind::King, Position{4, 6});
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(1, Position{4, 4}, Position{4, 6});

    const ArbiterResult arrival = arbiter.advanceTime(2000);
    CHECK(arrival.kingCaptured);
    CHECK(king->getState() == State::Captured);

    // The motion is cleared; a later advance does not re-report the capture.
    const ArbiterResult after = arbiter.advanceTime(1000);
    CHECK_FALSE(after.kingCaptured);
}

TEST_CASE("hasActiveMotion tracks the motion lifecycle") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    CHECK_FALSE(arbiter.hasActiveMotion());
    arbiter.startMotion(1, Position{4, 4}, Position{4, 5});
    CHECK(arbiter.hasActiveMotion());
    arbiter.advanceTime(1000);
    CHECK_FALSE(arbiter.hasActiveMotion());
}

TEST_CASE("startMotion enforces one active motion per piece") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(1, Position{4, 4}, Position{4, 5});

    SUBCASE("same piece again throws logic_error") {
        CHECK_THROWS_AS(arbiter.startMotion(1, Position{4, 4}, Position{4, 6}),
                        std::logic_error);
    }
    SUBCASE("a different piece may move simultaneously") {
        place(board, 2, Color::White, Kind::Rook, Position{6, 6});
        CHECK_NOTHROW(arbiter.startMotion(2, Position{6, 6}, Position{6, 7}));
        CHECK(arbiter.hasActiveMotion());

        arbiter.advanceTime(1000);  // both are 1-cell / 1000ms moves
        CHECK_FALSE(arbiter.hasActiveMotion());
        CHECK(board.getPieceAt(Position{4, 5}).get() != nullptr);
        CHECK(board.getPieceAt(Position{6, 7}).get() != nullptr);
    }
}

TEST_CASE("startMotion rejects a source with no matching piece") {
    Board board{8, 8};
    place(board, 1, Color::White, Kind::Rook, Position{4, 4});
    RealTimeArbiter arbiter{board};

    SUBCASE("empty source") {
        CHECK_THROWS_AS(arbiter.startMotion(9, Position{0, 0}, Position{0, 1}),
                        std::invalid_argument);
    }
    SUBCASE("id does not match the piece at source") {
        CHECK_THROWS_AS(arbiter.startMotion(99, Position{4, 4}, Position{4, 5}),
                        std::invalid_argument);
    }
}
