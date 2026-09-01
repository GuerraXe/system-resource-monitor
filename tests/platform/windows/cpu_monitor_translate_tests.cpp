#include "platform/windows/cpu_monitor.hpp"
#include "support/test_framework.hpp"

using srm::core::ErrorCode;
using srm::platform::windows::RawCpuTimes;
using srm::platform::windows::translate;

TEST_CASE("cpu translate: half idle over the interval is 50% utilization") {
    RawCpuTimes prev;
    prev.succeeded = true;
    prev.idle_ticks = 0;
    prev.total_ticks = 0;

    RawCpuTimes curr;
    curr.succeeded = true;
    curr.idle_ticks = 500;
    curr.total_ticks = 1000;

    auto result = translate(prev, curr);

    CHECK(static_cast<bool>(result));
    CHECK(result.value().total_utilization_percent == 50.0);
    CHECK(result.value().per_core_utilization_percent.empty());
}

TEST_CASE("cpu translate: no prior successful sample is Unavailable, not a crash") {
    RawCpuTimes prev; // succeeded defaults to false
    RawCpuTimes curr;
    curr.succeeded = true;
    curr.idle_ticks = 500;
    curr.total_ticks = 1000;

    auto result = translate(prev, curr);

    CHECK(!static_cast<bool>(result));
    CHECK(result.error().code == ErrorCode::Unavailable);
}

TEST_CASE("cpu translate: a failed current read is PlatformApiFailure with the error code in the message") {
    RawCpuTimes prev;
    prev.succeeded = true;

    RawCpuTimes curr;
    curr.succeeded = false;
    curr.last_error = 87; // ERROR_INVALID_PARAMETER

    auto result = translate(prev, curr);

    CHECK(!static_cast<bool>(result));
    CHECK(result.error().code == ErrorCode::PlatformApiFailure);
    CHECK(result.error().message.find("87") != std::string::npos);
}
