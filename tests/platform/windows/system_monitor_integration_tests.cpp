#include "platform/windows/system_monitor.hpp"
#include "support/test_framework.hpp"

// Real API calls. Asserts invariants, not exact values.

TEST_CASE("system monitor integration: real sample reports sane invariants") {
    srm::platform::windows::SystemMonitor monitor;
    auto result = monitor.sample();

    CHECK(static_cast<bool>(result));
    const auto& info = result.value();
    CHECK(!info.hostname.empty());
    CHECK(info.uptime.count() >= 0);
}
