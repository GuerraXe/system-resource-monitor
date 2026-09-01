#include "platform/windows/system_monitor.hpp"
#include "support/test_framework.hpp"

using srm::platform::windows::RawSystemInfo;
using srm::platform::windows::translate;

TEST_CASE("system translate: successful hostname query and uptime conversion") {
    RawSystemInfo raw;
    raw.hostname_query_succeeded = true;
    raw.hostname = "DESKTOP-TEST";
    raw.uptime_millis = 90'065'000; // 1d 1h 1m 5s

    auto result = translate(raw);

    CHECK(static_cast<bool>(result));
    CHECK_EQ(result.value().hostname, std::string("DESKTOP-TEST"));
    CHECK(result.value().uptime == std::chrono::seconds(90065));
}

TEST_CASE("system translate: a failed hostname query degrades to an empty hostname, not a failure") {
    RawSystemInfo raw;
    raw.hostname_query_succeeded = false;
    raw.uptime_millis = 5000;

    auto result = translate(raw);

    CHECK(static_cast<bool>(result));
    CHECK_EQ(result.value().hostname, std::string(""));
    CHECK(result.value().uptime == std::chrono::seconds(5));
}
