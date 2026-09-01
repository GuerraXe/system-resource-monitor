#include "support/test_framework.hpp"

// Milestone 0: proves the test binary builds, links, registers a case, and
// is discovered by CTest. Replaced/expanded by real coverage starting with
// core:: unit tests in Milestone 1.
TEST_CASE("test harness self-check") {
    CHECK(1 + 1 == 2);
    CHECK_EQ(std::string("srm"), std::string("srm"));
}
