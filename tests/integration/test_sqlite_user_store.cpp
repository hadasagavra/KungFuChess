#include "third_party/doctest/doctest.h"

#include "server/store/include/sqlite_user_store.hpp"
#include "shared/logic/game_record/include/rating.hpp"

using kfc::game_record::defaultRating;
using kfc::server::AuthResult;
using kfc::server::SqliteUserStore;

namespace {

// A private, throwaway database: ":memory:" lives only as long as the handle, so
// each test starts from an empty users table with no file to clean up.
SqliteUserStore freshStore() { return SqliteUserStore{":memory:"}; }

}  // namespace

TEST_CASE("an unknown user is registered at the starting rating and accepted") {
    SqliteUserStore store = freshStore();

    const AuthResult result = store.authenticate("alice", "secret");
    CHECK(result.accepted);
    CHECK(result.rating == defaultRating);
}

TEST_CASE("a returning user with the right password is accepted") {
    SqliteUserStore store = freshStore();
    store.authenticate("alice", "secret");  // register

    const AuthResult again = store.authenticate("alice", "secret");
    CHECK(again.accepted);
    CHECK(again.rating == defaultRating);
}

TEST_CASE("a wrong password is refused") {
    SqliteUserStore store = freshStore();
    store.authenticate("alice", "secret");  // register

    const AuthResult wrong = store.authenticate("alice", "guess");
    CHECK_FALSE(wrong.accepted);
}

TEST_CASE("a rating update persists across logins") {
    SqliteUserStore store = freshStore();
    store.authenticate("alice", "secret");  // register at 1200

    store.updateRating("alice", 1275);

    const AuthResult after = store.authenticate("alice", "secret");
    CHECK(after.accepted);
    CHECK(after.rating == 1275);
}
