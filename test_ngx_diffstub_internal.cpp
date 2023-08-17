#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "ngx_diffstub_internal.hpp" // Include your library header



TEST_CASE("Addition") {
    int result = 2 + 3;
    REQUIRE(result == 5);
}

TEST_CASE("Subtraction") {
    int result = 5 - 3;
    REQUIRE(result == 2);
}

#if 0
TEST_CASE("NotATest") {
    REQUIRE(1 == 1);
}
#endif