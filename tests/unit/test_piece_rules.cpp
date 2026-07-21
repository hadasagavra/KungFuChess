#include "third_party/doctest/doctest.h"

#include <cstdint>
#include <memory>
#include <set>

#include "shared/logic/model/include/board.hpp"
#include "shared/logic/model/include/piece.hpp"
#include "shared/logic/model/include/position.hpp"
#include "shared/logic/rules/include/piece_rules.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Kind;
using kfc::model::Piece;
using kfc::model::Position;
using kfc::rules::BishopRules;
using kfc::rules::KingRules;
using kfc::rules::KnightRules;
using kfc::rules::PawnRules;
using kfc::rules::QueenRules;
using kfc::rules::RookRules;

namespace {

std::shared_ptr<Piece> place(Board& board, std::uint32_t id, Color color,
                             Kind kind, Position cell) {
    auto piece = std::make_shared<Piece>(id, color, kind, cell);
    board.addPiece(piece);
    return piece;
}

bool contains(const std::set<Position>& dests, Position pos) {
    return dests.count(pos) == 1;
}

}  // namespace

TEST_CASE("RookRules slides orthogonally and respects obstructions") {
    SUBCASE("empty board from the centre") {
        Board board{8, 8};
        auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});

        const auto dests = RookRules{}.legalDestinations(board, *rook);

        CHECK(dests.size() == 14);
        CHECK(contains(dests, Position{4, 0}));
        CHECK(contains(dests, Position{4, 7}));
        CHECK(contains(dests, Position{0, 4}));
        CHECK(contains(dests, Position{7, 4}));
        CHECK_FALSE(contains(dests, Position{4, 4}));  // not its own square
        CHECK_FALSE(contains(dests, Position{5, 5}));  // diagonals excluded
    }
    SUBCASE("stops before a friendly piece") {
        Board board{8, 8};
        auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{4, 6});

        const auto dests = RookRules{}.legalDestinations(board, *rook);

        CHECK(contains(dests, Position{4, 5}));
        CHECK_FALSE(contains(dests, Position{4, 6}));  // friendly blocks
        CHECK_FALSE(contains(dests, Position{4, 7}));  // cannot pass
    }
    SUBCASE("captures an enemy but cannot pass through it") {
        Board board{8, 8};
        auto rook = place(board, 1, Color::White, Kind::Rook, Position{4, 4});
        place(board, 2, Color::Black, Kind::Pawn, Position{4, 6});

        const auto dests = RookRules{}.legalDestinations(board, *rook);

        CHECK(contains(dests, Position{4, 5}));
        CHECK(contains(dests, Position{4, 6}));         // enemy is capturable
        CHECK_FALSE(contains(dests, Position{4, 7}));   // cannot pass beyond
    }
}

TEST_CASE("BishopRules slides diagonally and respects obstructions") {
    SUBCASE("empty board from the centre") {
        Board board{8, 8};
        auto bishop = place(board, 1, Color::White, Kind::Bishop, Position{4, 4});

        const auto dests = BishopRules{}.legalDestinations(board, *bishop);

        CHECK(dests.size() == 13);
        CHECK(contains(dests, Position{7, 7}));
        CHECK(contains(dests, Position{0, 0}));
        CHECK_FALSE(contains(dests, Position{4, 5}));  // orthogonals excluded
    }
    SUBCASE("stops before a friendly piece") {
        Board board{8, 8};
        auto bishop = place(board, 1, Color::White, Kind::Bishop, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{6, 6});

        const auto dests = BishopRules{}.legalDestinations(board, *bishop);

        CHECK(contains(dests, Position{5, 5}));
        CHECK_FALSE(contains(dests, Position{6, 6}));
        CHECK_FALSE(contains(dests, Position{7, 7}));
    }
    SUBCASE("captures an enemy but cannot pass through it") {
        Board board{8, 8};
        auto bishop = place(board, 1, Color::White, Kind::Bishop, Position{4, 4});
        place(board, 2, Color::Black, Kind::Pawn, Position{6, 6});

        const auto dests = BishopRules{}.legalDestinations(board, *bishop);

        CHECK(contains(dests, Position{5, 5}));
        CHECK(contains(dests, Position{6, 6}));
        CHECK_FALSE(contains(dests, Position{7, 7}));
    }
}

TEST_CASE("QueenRules combines rook and bishop movement") {
    SUBCASE("empty board from the centre") {
        Board board{8, 8};
        auto queen = place(board, 1, Color::White, Kind::Queen, Position{4, 4});

        const auto dests = QueenRules{}.legalDestinations(board, *queen);

        CHECK(dests.size() == 27);  // 14 orthogonal + 13 diagonal
        CHECK(contains(dests, Position{4, 0}));  // orthogonal
        CHECK(contains(dests, Position{0, 0}));  // diagonal
    }
    SUBCASE("friendly blocks and enemy is capturable") {
        Board board{8, 8};
        auto queen = place(board, 1, Color::White, Kind::Queen, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{4, 6});
        place(board, 3, Color::Black, Kind::Pawn, Position{6, 6});

        const auto dests = QueenRules{}.legalDestinations(board, *queen);

        CHECK_FALSE(contains(dests, Position{4, 6}));  // friendly blocks
        CHECK(contains(dests, Position{6, 6}));        // enemy capture
        CHECK_FALSE(contains(dests, Position{7, 7}));  // cannot pass enemy
    }
}

TEST_CASE("KnightRules jumps in an L and ignores obstructions") {
    SUBCASE("empty board from the centre") {
        Board board{8, 8};
        auto knight = place(board, 1, Color::White, Kind::Knight, Position{4, 4});

        const auto dests = KnightRules{}.legalDestinations(board, *knight);

        CHECK(dests.size() == 8);
        CHECK(contains(dests, Position{6, 5}));
        CHECK(contains(dests, Position{2, 3}));
    }
    SUBCASE("corner limits the number of moves") {
        Board board{8, 8};
        auto knight = place(board, 1, Color::White, Kind::Knight, Position{0, 0});

        const auto dests = KnightRules{}.legalDestinations(board, *knight);

        CHECK(dests.size() == 2);
        CHECK(contains(dests, Position{2, 1}));
        CHECK(contains(dests, Position{1, 2}));
    }
    SUBCASE("jumps over a ring of surrounding pieces") {
        Board board{8, 8};
        auto knight = place(board, 1, Color::White, Kind::Knight, Position{4, 4});
        std::uint32_t id = 2;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                place(board, id++, Color::White, Kind::Pawn,
                      Position{4 + dr, 4 + dc});
            }
        }

        const auto dests = KnightRules{}.legalDestinations(board, *knight);

        CHECK(dests.size() == 8);  // surrounding ring is jumped over
    }
    SUBCASE("landing square: friendly excluded, enemy captured") {
        Board friendlyBoard{8, 8};
        auto k1 = place(friendlyBoard, 1, Color::White, Kind::Knight, Position{4, 4});
        place(friendlyBoard, 2, Color::White, Kind::Pawn, Position{6, 5});
        const auto friendlyDests = KnightRules{}.legalDestinations(friendlyBoard, *k1);
        CHECK(friendlyDests.size() == 7);
        CHECK_FALSE(contains(friendlyDests, Position{6, 5}));

        Board enemyBoard{8, 8};
        auto k2 = place(enemyBoard, 1, Color::White, Kind::Knight, Position{4, 4});
        place(enemyBoard, 2, Color::Black, Kind::Pawn, Position{6, 5});
        const auto enemyDests = KnightRules{}.legalDestinations(enemyBoard, *k2);
        CHECK(enemyDests.size() == 8);
        CHECK(contains(enemyDests, Position{6, 5}));
    }
}

TEST_CASE("KingRules moves one square in any direction") {
    SUBCASE("empty board from the centre") {
        Board board{8, 8};
        auto king = place(board, 1, Color::White, Kind::King, Position{4, 4});

        const auto dests = KingRules{}.legalDestinations(board, *king);

        CHECK(dests.size() == 8);
    }
    SUBCASE("corner limits movement to three squares") {
        Board board{8, 8};
        auto king = place(board, 1, Color::White, Kind::King, Position{0, 0});

        const auto dests = KingRules{}.legalDestinations(board, *king);

        CHECK(dests.size() == 3);
        CHECK(contains(dests, Position{0, 1}));
        CHECK(contains(dests, Position{1, 0}));
        CHECK(contains(dests, Position{1, 1}));
    }
    SUBCASE("friendly adjacent excluded, enemy adjacent captured") {
        Board board{8, 8};
        auto king = place(board, 1, Color::White, Kind::King, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{4, 5});
        place(board, 3, Color::Black, Kind::Pawn, Position{5, 5});

        const auto dests = KingRules{}.legalDestinations(board, *king);

        CHECK_FALSE(contains(dests, Position{4, 5}));  // friendly
        CHECK(contains(dests, Position{5, 5}));        // enemy capture
    }
}

TEST_CASE("PawnRules moves forward and captures diagonally") {
    SUBCASE("white advances one square upward on an empty board") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{4, 4});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.size() == 1);
        CHECK(contains(dests, Position{3, 4}));        // one square up (row - 1)
        CHECK_FALSE(contains(dests, Position{5, 4}));  // never backward
        CHECK_FALSE(contains(dests, Position{2, 4}));  // never two squares
    }
    SUBCASE("forward is blocked by any piece") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{4, 4});
        place(board, 2, Color::Black, Kind::Pawn, Position{3, 4});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.empty());  // cannot capture forward, cannot advance
    }
    SUBCASE("captures diagonally forward but not straight ahead") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{4, 4});
        place(board, 2, Color::Black, Kind::Pawn, Position{3, 3});
        place(board, 3, Color::Black, Kind::Pawn, Position{3, 5});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.size() == 3);
        CHECK(contains(dests, Position{3, 4}));  // forward (empty)
        CHECK(contains(dests, Position{3, 3}));  // diagonal capture
        CHECK(contains(dests, Position{3, 5}));  // diagonal capture
    }
    SUBCASE("a friendly diagonal is not a capture") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{3, 3});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK_FALSE(contains(dests, Position{3, 3}));
        CHECK(contains(dests, Position{3, 4}));  // forward still available
    }
    SUBCASE("black advances one square downward and captures diagonally") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::Black, Kind::Pawn, Position{4, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{5, 3});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(contains(dests, Position{5, 4}));         // one square down (row + 1)
        CHECK_FALSE(contains(dests, Position{3, 4}));   // never backward (upward)
        CHECK(contains(dests, Position{5, 3}));         // diagonal capture
    }
}

TEST_CASE("PawnRules allows a double step from the start row when the path is clear") {
    SUBCASE("white double step from its start row (height - 2)") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{6, 4});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.size() == 2);
        CHECK(contains(dests, Position{5, 4}));        // single
        CHECK(contains(dests, Position{4, 4}));        // double
        CHECK_FALSE(contains(dests, Position{3, 4}));  // never a triple step
    }
    SUBCASE("black double step from its start row (1)") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::Black, Kind::Pawn, Position{1, 4});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.size() == 2);
        CHECK(contains(dests, Position{2, 4}));
        CHECK(contains(dests, Position{3, 4}));
    }
    SUBCASE("second square blocked: only the single step") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{6, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{4, 4});  // blocks the double

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.size() == 1);
        CHECK(contains(dests, Position{5, 4}));
        CHECK_FALSE(contains(dests, Position{4, 4}));
    }
    SUBCASE("first square blocked: no forward move at all") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{6, 4});
        place(board, 2, Color::White, Kind::Pawn, Position{5, 4});  // blocks the path

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.empty());
    }
    SUBCASE("not on the start row: single step only") {
        Board board{8, 8};
        auto pawn = place(board, 1, Color::White, Kind::Pawn, Position{4, 4});

        const auto dests = PawnRules{}.legalDestinations(board, *pawn);

        CHECK(dests.size() == 1);
        CHECK(contains(dests, Position{3, 4}));
    }
}
