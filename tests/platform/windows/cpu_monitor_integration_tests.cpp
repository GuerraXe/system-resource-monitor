#include <chrono>
#include <thread>

#include "platform/windows/cpu_monitor.hpp"
#include "support/test_framework.hpp"

// Real API calls. Asserts invariants, not an exact utilization figure --
// there's no fixed "correct" CPU load on the machine running this test.

TEST_CASE("cpu monitor integration: real sample after a brief interval is a sane percentage") {
    srm::platform::windows::CpuMonitor monitor; // takes its baseline reading now
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto result = monitor.sample();

    CHECK(static_cast<bool>(result));
    const auto& cpu = result.value();
    CHECK(cpu.total_utilization_percent >= 0.0);
    CHECK(cpu.total_utilization_percent <= 100.0);
}
