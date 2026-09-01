#include "core/result.hpp"
#include "support/test_framework.hpp"

using srm::core::Error;
using srm::core::ErrorCode;
using srm::core::Result;

TEST_CASE("Result: Ok carries a value and reports success") {
    auto r = Result<int>::Ok(42);
    CHECK(static_cast<bool>(r));
    CHECK(r.has_value());
    CHECK_EQ(r.value(), 42);
}

TEST_CASE("Result: Fail carries an error and reports failure") {
    auto r = Result<int>::Fail(Error{ErrorCode::Unavailable, "no such counter"});
    CHECK(!static_cast<bool>(r));
    CHECK(!r.has_value());
    // CHECK, not CHECK_EQ: ErrorCode is a scoped enum with no operator<<,
    // and CHECK_EQ's failure-path streaming has to compile even when the
    // check passes.
    CHECK(r.error().code == ErrorCode::Unavailable);
    CHECK_EQ(r.error().message, std::string("no such counter"));
}

TEST_CASE("Result: value_or returns the value on success") {
    auto r = Result<int>::Ok(7);
    CHECK_EQ(r.value_or(-1), 7);
}

TEST_CASE("Result: value_or returns the fallback on failure") {
    auto r = Result<int>::Fail(Error{ErrorCode::NotFound, "process exited"});
    CHECK_EQ(r.value_or(-1), -1);
}

TEST_CASE("Result: calling value() on a failure throws") {
    auto r = Result<int>::Fail(Error{ErrorCode::PlatformApiFailure, "boom"});
    bool threw = false;
    try {
        (void)r.value();
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE("Result: calling error() on a success throws") {
    auto r = Result<int>::Ok(1);
    bool threw = false;
    try {
        (void)r.error();
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
}
