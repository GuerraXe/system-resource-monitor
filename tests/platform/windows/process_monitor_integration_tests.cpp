#include <chrono>
#include <thread>

#include "platform/windows/process_monitor.hpp"
#include "support/test_framework.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// Real API calls. Asserts invariants, not exact values -- the process list
// varies by machine and moment.

TEST_CASE("process monitor integration: real sample contains this test process") {
    srm::platform::windows::ProcessMonitor monitor;
    auto result = monitor.sample();

    CHECK(static_cast<bool>(result));
    const auto& processes = result.value();
    CHECK(!processes.empty());

    const auto self_pid = static_cast<std::uint32_t>(GetCurrentProcessId());
    bool found_self = false;
    for (const auto& p : processes) {
        if (p.pid == self_pid) {
            found_self = true;
        }
    }
    CHECK(found_self);
}

TEST_CASE("process monitor integration: a second sample after a brief interval reports sane CPU percentages") {
    srm::platform::windows::ProcessMonitor monitor;
    auto first = monitor.sample();
    CHECK(static_cast<bool>(first));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto second = monitor.sample();
    CHECK(static_cast<bool>(second));
    for (const auto& p : second.value()) {
        // Not clamped to 100 by design (see core::math::cpu_percent_of_wall_time),
        // but should never be negative.
        CHECK(p.cpu_percent >= 0.0);
    }
}
