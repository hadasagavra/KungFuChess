#pragma once

#include <string>

namespace kfc::server {

// What a login attempt resolved to: whether the credentials were accepted, and
// the player's current rating. A rejected attempt (an existing name with the
// wrong password) carries the default rating but must not be acted on.
struct AuthResult {
    bool accepted;
    int rating;
};

// Where player accounts live, and nothing more. It stores a name, a password
// token, and a rating, and it never names a game concept -- so the session can
// be driven by a real SQLite file, an in-memory map, or a test double without
// changing a line. This is the seam that keeps the session testable without a
// database, mirroring MessageTransport for the network.
class UserStore {
public:
    virtual ~UserStore() = default;

    // Log a player in, registering them on first sight. An unknown username is
    // created with the given password and the default rating and accepted. A
    // known username is accepted only if the password matches; either way the
    // stored rating comes back.
    virtual AuthResult authenticate(const std::string& username,
                                    const std::string& password) = 0;

    // Persist a new rating for an existing account (after a game).
    virtual void updateRating(const std::string& username, int rating) = 0;
};

}  // namespace kfc::server
