#include <cstring>

#include <doctest/doctest.h>
#include <nme/version.h>

TEST_CASE("engine_version returns a non-empty, sensible string") {
    const char* v = nme::engine_version();
    REQUIRE(v != nullptr);
    CHECK(std::strlen(v) > 0);
    CHECK(std::strstr(v, "nme") != nullptr);
}