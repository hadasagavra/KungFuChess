#include "third_party/doctest/doctest.h"

#include <string>

#include "server/store/include/password_hasher.hpp"

using kfc::server::hashPassword;

TEST_CASE("the SHA-256 core matches a known answer") {
    // hashPassword folds username and password with a ':' separator, so this is
    // the SHA-256 of "user:secret". The expected digest is an independent
    // reference (coreutils sha256sum), so a wrong constant in the core is caught.
    const std::string digest = hashPassword("user", "secret");
    CHECK(digest ==
          "92592125f3859823818804f00932aca5b658d7a334a5feaa8ab7fa321702e913");
    CHECK(digest.size() == 64);  // 256 bits as lowercase hex
}

TEST_CASE("hashing is deterministic") {
    CHECK(hashPassword("alice", "pw") == hashPassword("alice", "pw"));
}

TEST_CASE("the password is not stored in the clear") {
    // The digest must not contain the raw password as a substring.
    const std::string digest = hashPassword("alice", "hunter2");
    CHECK(digest.find("hunter2") == std::string::npos);
}

TEST_CASE("a different password gives a different hash") {
    CHECK(hashPassword("alice", "pw1") != hashPassword("alice", "pw2"));
}

TEST_CASE("the username salts the hash") {
    // Same password, different user: the folded-in username keeps the two apart.
    CHECK(hashPassword("alice", "shared") != hashPassword("bob", "shared"));
}

TEST_CASE("the separator prevents field-boundary collisions") {
    // Without a separator "ab"+"c" and "a"+"bc" would both hash "abc"; the
    // ':' between the fields keeps them distinct.
    CHECK(hashPassword("ab", "c") != hashPassword("a", "bc"));
}
