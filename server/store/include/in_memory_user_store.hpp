#pragma once

#include <map>
#include <string>

#include "server/store/include/user_store.hpp"

namespace kfc::server {

// A UserStore kept entirely in memory, with no database behind it. It holds the
// same password hashing and register-or-verify rule as the SQLite store, so it
// behaves identically -- it simply forgets everything when it is destroyed. It is
// what local (loopback) play and the tests use, so neither drags SQLite in.
class InMemoryUserStore : public UserStore {
public:
    AuthResult authenticate(const std::string& username,
                            const std::string& password) override;
    void updateRating(const std::string& username, int rating) override;

private:
    struct Account {
        std::string passwordHash;
        int rating;
    };
    std::map<std::string, Account> accounts_;
};

}  // namespace kfc::server
