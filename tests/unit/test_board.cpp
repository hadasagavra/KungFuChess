#include "third_party/doctest/doctest.h"

#include <memory>
#include <stdexcept>

#include "model/include/board.hpp"
#include "model/include/piece.hpp"
#include "model/include/position.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;

namespace {

std::shared_ptr<Piece> makePiece(std::uint32_t id, Position cell) {
    return std::make_shared<Piece>(id, Color::White, Kind::Pawn, cell);
}

}  // namespace

TEST_CASE("Board reports dimensions and bounds") {
    const Board b{8, 8};

    CHECK(b.width() == 8);
    CHECK(b.height() == 8);

    CHECK(b.isInsideBounds(Position{0, 0}));
    CHECK(b.isInsideBounds(Position{7, 7}));

    CHECK_FALSE(b.isInsideBounds(Position{-1, 0}));
    CHECK_FALSE(b.isInsideBounds(Position{0, -1}));
    CHECK_FALSE(b.isInsideBounds(Position{8, 0}));
    CHECK_FALSE(b.isInsideBounds(Position{0, 8}));

    SUBCASE("non-square board keeps row and col mapping distinct") {
        const Board rect{5, 3};  // width = 5 columns, height = 3 rows
        CHECK(rect.width() == 5);
        CHECK(rect.height() == 3);
        CHECK(rect.isInsideBounds(Position{2, 4}));        // row 2 < 3, col 4 < 5
        CHECK_FALSE(rect.isInsideBounds(Position{4, 2}));  // row 4 >= 3
    }
}

TEST_CASE("Board distinguishes empty and occupied cells") {
    Board b{8, 8};

    CHECK_FALSE(b.isOccupied(Position{2, 3}));
    CHECK(b.getPieceAt(Position{2, 3}).get() == nullptr);

    auto piece = makePiece(1, Position{2, 3});
    b.addPiece(piece);

    CHECK(b.isOccupied(Position{2, 3}));
    CHECK(b.getPieceAt(Position{2, 3}).get() == piece.get());
    CHECK(b.getPieceAt(Position{2, 3})->getId() == 1u);
    CHECK_FALSE(b.isOccupied(Position{0, 0}));
}

TEST_CASE("Board::addPiece rejects invalid placements") {
    Board b{8, 8};
    b.addPiece(makePiece(1, Position{4, 4}));

    SUBCASE("double occupancy throws") {
        CHECK_THROWS_AS(b.addPiece(makePiece(2, Position{4, 4})), std::invalid_argument);
    }
    SUBCASE("out-of-bounds cell throws") {
        CHECK_THROWS_AS(b.addPiece(makePiece(3, Position{8, 8})), std::invalid_argument);
    }
    SUBCASE("null piece throws") {
        CHECK_THROWS_AS(b.addPiece(nullptr), std::invalid_argument);
    }
}

TEST_CASE("Board::movePiece clears origin, updates destination, and syncs the piece") {
    Board b{8, 8};
    auto piece = makePiece(1, Position{1, 1});
    b.addPiece(piece);

    b.movePiece(Position{1, 1}, Position{3, 4});

    CHECK_FALSE(b.isOccupied(Position{1, 1}));
    CHECK(b.getPieceAt(Position{1, 1}).get() == nullptr);
    CHECK(b.isOccupied(Position{3, 4}));
    CHECK(b.getPieceAt(Position{3, 4}).get() == piece.get());
    CHECK(piece->getCell() == Position{3, 4});

    SUBCASE("moving onto an occupied cell throws") {
        b.addPiece(makePiece(2, Position{5, 5}));
        CHECK_THROWS_AS(b.movePiece(Position{3, 4}, Position{5, 5}), std::invalid_argument);
    }
    SUBCASE("moving from an empty cell throws") {
        CHECK_THROWS_AS(b.movePiece(Position{0, 0}, Position{0, 1}), std::invalid_argument);
    }
}

TEST_CASE("Board::removePiece empties the cell") {
    Board b{8, 8};
    b.addPiece(makePiece(1, Position{6, 6}));
    REQUIRE(b.isOccupied(Position{6, 6}));

    b.removePiece(Position{6, 6});

    CHECK_FALSE(b.isOccupied(Position{6, 6}));
    CHECK(b.getPieceAt(Position{6, 6}).get() == nullptr);

    SUBCASE("removing an already-empty cell is a no-op") {
        CHECK_NOTHROW(b.removePiece(Position{0, 0}));
    }
}

TEST_CASE("Board throws out_of_range on out-of-bounds access") {
    Board b{8, 8};

    CHECK_THROWS_AS(b.isOccupied(Position{8, 8}), std::out_of_range);
    CHECK_THROWS_AS(b.getPieceAt(Position{-1, 0}), std::out_of_range);
    CHECK_THROWS_AS(b.removePiece(Position{9, 9}), std::out_of_range);
    CHECK_THROWS_AS(b.movePiece(Position{0, 0}, Position{9, 9}), std::out_of_range);
}
