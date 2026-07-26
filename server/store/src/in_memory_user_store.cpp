#include "server/store/include/in_memory_user_store.hpp"

#include "server/store/include/password_hasher.hpp"
#include "shared/logic/game_record/include/rating.hpp"

namespace kfc::server {

AuthResult InMemoryUserStore::authenticate(const std::string& username,
                                           const std::string& password) {
    const std::string hash = hashPassword(username, password);
    const auto found = accounts_.find(username);
    if (found == accounts_.end()) {
        // First sight: register at the starting rating and let them in.
        accounts_[username] = Account{hash, game_record::defaultRating};
        return AuthResult{true, game_record::defaultRating};
    }
    // Known name: only the matching password is accepted; the rating stands.
    const bool accepted = found->second.passwordHash == hash;
    return AuthResult{accepted, found->second.rating};
}

void InMemoryUserStore::updateRating(const std::string& username, int rating) {
    const auto found = accounts_.find(username);
    if (found != accounts_.end()) found->second.rating = rating;
}

}  // namespace kfc::server
