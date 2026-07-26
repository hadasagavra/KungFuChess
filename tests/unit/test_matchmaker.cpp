#include "third_party/doctest/doctest.h"

#include "server/app/include/matchmaker.hpp"

using kfc::server::Matchmaker;

TEST_CASE("two seekers within range are paired") {
    Matchmaker matchmaker;
    CHECK_FALSE(matchmaker.seek(1, 1200));  // no one waiting yet: queued
    const std::optional<kfc::server::ClientId> match = matchmaker.seek(2, 1250);
    REQUIRE(match);
    CHECK(*match == 1);
    CHECK_FALSE(matchmaker.isWaiting(1));  // removed on match
    CHECK_FALSE(matchmaker.isWaiting(2));  // never queued
}

TEST_CASE("seekers outside the range are not paired") {
    Matchmaker matchmaker;
    CHECK_FALSE(matchmaker.seek(1, 1200));
    CHECK_FALSE(matchmaker.seek(2, 1400));  // 200 apart: both wait
    CHECK(matchmaker.isWaiting(1));
    CHECK(matchmaker.isWaiting(2));
}

TEST_CASE("the range is inclusive at its boundary") {
    Matchmaker matchmaker;
    matchmaker.seek(1, 1200);
    const std::optional<kfc::server::ClientId> match = matchmaker.seek(2, 1300);
    REQUIRE(match);  // exactly 100 apart still matches
    CHECK(*match == 1);
}

TEST_CASE("a search times out after a minute") {
    Matchmaker matchmaker;
    matchmaker.seek(1, 1200);
    CHECK(matchmaker.tick(59000).empty());  // still within the minute
    const std::vector<kfc::server::ClientId> expired = matchmaker.tick(2000);
    REQUIRE(expired.size() == 1);
    CHECK(expired[0] == 1);
    CHECK_FALSE(matchmaker.isWaiting(1));
}

TEST_CASE("a cancelled search leaves the queue") {
    Matchmaker matchmaker;
    matchmaker.seek(1, 1200);
    matchmaker.cancel(1);
    CHECK_FALSE(matchmaker.isWaiting(1));
    CHECK(matchmaker.tick(60000).empty());  // nothing left to time out
}
