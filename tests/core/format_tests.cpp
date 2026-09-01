#include "core/format.hpp"
#include "support/test_framework.hpp"

using namespace srm::core::format;

TEST_CASE("format::bytes: sub-1024 values stay in bytes with no decimal") {
    CHECK_EQ(bytes(0), std::string("0 B"));
    CHECK_EQ(bytes(512), std::string("512 B"));
}

TEST_CASE("format::bytes: scales to KB/MB/GB with one decimal place") {
    CHECK_EQ(bytes(1536), std::string("1.5 KB"));
    CHECK_EQ(bytes(1024ull * 1024 * 3), std::string("3.0 MB"));
    CHECK_EQ(bytes(1024ull * 1024 * 1024 * 2), std::string("2.0 GB"));
}

TEST_CASE("format::percent: default one decimal place") {
    CHECK_EQ(percent(42.5), std::string("42.5%"));
}

TEST_CASE("format::percent: custom decimal precision") {
    CHECK_EQ(percent(42.567, 2), std::string("42.57%"));
    CHECK_EQ(percent(42.567, 0), std::string("43%"));
}

TEST_CASE("format::duration_seconds: sub-minute shows only seconds") {
    CHECK_EQ(duration_seconds(45), std::string("45s"));
}

TEST_CASE("format::duration_seconds: sub-hour shows minutes and seconds") {
    CHECK_EQ(duration_seconds(125), std::string("2m 5s"));
}

TEST_CASE("format::duration_seconds: over an hour includes hours") {
    CHECK_EQ(duration_seconds(3725), std::string("1h 2m 5s"));
}

TEST_CASE("format::duration_seconds: over a day includes days") {
    CHECK_EQ(duration_seconds(90065), std::string("1d 1h 1m 5s"));
}

TEST_CASE("format::duration_seconds: zero is a plain 0s") {
    CHECK_EQ(duration_seconds(0), std::string("0s"));
}
