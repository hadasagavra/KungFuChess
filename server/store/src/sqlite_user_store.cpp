// winsqlite3.h publishes the SQLite API only for Windows 10 or greater. The
// vendored WebSocket stack pins _WIN32_WINNT to 0x0601 (Windows 7) build-wide,
// which would hide that API here, so this one translation unit raises the target
// version before any header is seen. It includes no WebSocket code, so nothing
// downstream depends on the lower version.
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000

#include "server/store/include/sqlite_user_store.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <winsqlite/winsqlite3.h>

#include "server/store/include/password_hasher.hpp"
#include "shared/logic/game_record/include/rating.hpp"

namespace kfc::server {
namespace {

// The one schema this store owns. A rating defaults to the starting value so a
// row can be inserted with only a name and a hash.
const char* const createTableSql =
    "CREATE TABLE IF NOT EXISTS users("
    "username TEXT PRIMARY KEY,"
    "password_hash TEXT NOT NULL,"
    "rating INTEGER NOT NULL)";

}  // namespace

struct SqliteUserStore::Impl {
    sqlite3* db = nullptr;

    // Prepare a statement or throw: a malformed SQL here is a programming error,
    // not a runtime condition to recover from.
    sqlite3_stmt* prepare(const std::string& sql) const {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("sqlite prepare failed: ") +
                                     sqlite3_errmsg(db));
        }
        return stmt;
    }

    // The stored {hash, rating} for a username, or nothing if no such row.
    std::optional<std::pair<std::string, int>> lookup(const std::string& username) {
        sqlite3_stmt* stmt =
            prepare("SELECT password_hash, rating FROM users WHERE username = ?");
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        std::optional<std::pair<std::string, int>> result;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* hash = sqlite3_column_text(stmt, 0);
            const int rating = sqlite3_column_int(stmt, 1);
            result = std::make_pair(
                std::string(reinterpret_cast<const char*>(hash)), rating);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    void insert(const std::string& username, const std::string& hash,
                int rating) {
        sqlite3_stmt* stmt = prepare(
            "INSERT INTO users(username, password_hash, rating) "
            "VALUES(?, ?, ?)");
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, rating);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
};

SqliteUserStore::SqliteUserStore(const std::string& path)
    : impl_(std::make_unique<Impl>()) {
    if (sqlite3_open(path.c_str(), &impl_->db) != SQLITE_OK) {
        throw std::runtime_error("cannot open user database: " + path);
    }
    if (sqlite3_exec(impl_->db, createTableSql, nullptr, nullptr, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(std::string("cannot create user table: ") +
                                 sqlite3_errmsg(impl_->db));
    }
}

SqliteUserStore::~SqliteUserStore() {
    if (impl_->db) sqlite3_close(impl_->db);
}

AuthResult SqliteUserStore::authenticate(const std::string& username,
                                         const std::string& password) {
    const std::string hash = hashPassword(username, password);
    const std::optional<std::pair<std::string, int>> existing =
        impl_->lookup(username);
    if (!existing) {
        // First sight: register at the starting rating and let them in.
        impl_->insert(username, hash, game_record::defaultRating);
        return AuthResult{true, game_record::defaultRating};
    }
    // Known name: only the matching password is accepted; the rating stands.
    const bool accepted = existing->first == hash;
    return AuthResult{accepted, existing->second};
}

void SqliteUserStore::updateRating(const std::string& username, int rating) {
    sqlite3_stmt* stmt =
        impl_->prepare("UPDATE users SET rating = ? WHERE username = ?");
    sqlite3_bind_int(stmt, 1, rating);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

}  // namespace kfc::server
