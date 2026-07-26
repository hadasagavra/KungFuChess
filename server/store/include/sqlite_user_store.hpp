#pragma once

#include <memory>
#include <string>

#include "server/store/include/user_store.hpp"

namespace kfc::server {

// A UserStore backed by a real SQLite database file. It is the production store
// the server runs; the SQLite handle and the winsqlite3 header live entirely
// behind a pimpl, so nothing that links this sees SQLite -- the same discipline
// the WebSocket transport keeps for asio. Opening ":memory:" gives a private
// throwaway database, which is what the tests use.
class SqliteUserStore : public UserStore {
public:
    explicit SqliteUserStore(const std::string& path);
    ~SqliteUserStore() override;

    AuthResult authenticate(const std::string& username,
                            const std::string& password) override;
    void updateRating(const std::string& username, int rating) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kfc::server
