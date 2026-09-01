#include "platform/windows/disk_monitor.hpp"
#include "support/test_framework.hpp"

// Real API call. Asserts invariants, not exact values -- volume layout
// varies by machine.

TEST_CASE("disk monitor integration: real sample reports sane invariants") {
    srm::platform::windows::DiskMonitor monitor;
    auto result = monitor.sample();

    CHECK(static_cast<bool>(result));
    const auto& volumes = result.value();
    // Every machine this runs on has at least one fixed volume (the one the
    // build lives on).
    CHECK(!volumes.empty());
    for (const auto& v : volumes) {
        CHECK(v.free_bytes <= v.total_bytes);
        CHECK(!v.mount_point.empty());
    }
}
