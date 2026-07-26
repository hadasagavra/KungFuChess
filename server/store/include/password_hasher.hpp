#pragma once

#include <string>

namespace kfc::server {

// Turn a username and password into the string stored in the user table. The
// password is never kept in the clear: this returns a SHA-256 digest, in
// lowercase hex, of the username and password joined by a separator. Folding the
// username in salts the digest per account, so two users with the same password
// do not share a hash. It is the one place a credential becomes a stored token,
// so the store and any test agree on exactly one hashing rule.
std::string hashPassword(const std::string& username,
                         const std::string& password);

}  // namespace kfc::server
