#include "platform/windows/memory_monitor.hpp"
#include "support/test_framework.hpp"

// Calls the real Windows API. Asserts invariants that must hold on any
// machine this runs on, rather than exact values -- there is no fixed
// "correct" amount of RAM to expect.

TEST_CASE("memory monitor integration: real sample reports sane invariants") {
    srm::platform::windows::MemoryMonitor monitor;
    auto result = monitor.sample();

    CHECK(static_cast<bool>(result));
    const auto& mem = result.value();
    CHECK(mem.total_physical_bytes > 0);
    CHECK(mem.available_physical_bytes <= mem.total_physical_bytes);
    CHECK(mem.available_page_file_bytes <= mem.total_page_file_bytes);
}
