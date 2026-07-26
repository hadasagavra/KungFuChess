#pragma once

namespace kfc::game_record {

// The Elo rating rule, as pure arithmetic: no game rules, no I/O, no accounts.
// A server computes updated ratings from a game's result and persists them; this
// is the single place the formula and its constants live, so nothing else
// re-derives them. It sits beside ScoreBoard as another product-feature record.

// Every new player starts here. Shared with the user store, which stamps it on a
// freshly created account, so the starting rating is defined exactly once.
constexpr int defaultRating = 1200;

// The maximum a single game can move a rating (the Elo K-factor).
constexpr int kFactor = 32;

// The expected score of `rating` against `opponentRating`, in [0, 1]: the
// probability-like share of a win the ratings alone predict. Equal ratings give
// 0.5; a higher rating gives more.
double expectedScore(int rating, int opponentRating);

// The rating after a game, rounded to a whole number: rating + K * (actual -
// expected), where actual is 1.0 for a win and 0.0 for a loss. A win raises the
// rating (more so against a stronger opponent), a loss lowers it.
int updatedRating(int rating, int opponentRating, double actualScore);

}  // namespace kfc::game_record
