#include <chrono>
#include <thread>

#include "platform/windows/network_monitor.hpp"
#include "support/test_framework.hpp"

// Real API calls. Asserts invariants, not exact values or interface
// presence -- a machine with no active adapters would legitimately report
// an empty list, so this only checks that whatever comes back is sane.

TEST_CASE("network monitor integration: real sample reports non-negative rates") {
    srm::platform::windows::NetworkMonitor monitor;
    monitor.sample(); // throwaway: populates the previous-bytes baseline

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto result = monitor.sample();
    CHECK(static_cast<bool>(result));
    for (const auto& iface : result.value()) {
        CHECK(iface.receive_bytes_per_second >= 0.0);
        CHECK(iface.send_bytes_per_second >= 0.0);
        CHECK(!iface.name.empty());
    }
}
