#include <iostream>

#include "core/format.hpp"
#include "platform/windows/memory_monitor.hpp"

// Placeholder entry point wired directly to one monitor as an end-to-end
// smoke check. Replaced by real CLI parsing, the monitor engine, and the
// presentation layer in later milestones.
int main() {
    srm::platform::windows::MemoryMonitor memory_monitor;
    const auto result = memory_monitor.sample();

    if (result) {
        const auto& mem = result.value();
        std::cout << "Physical memory: "
                  << srm::core::format::bytes(mem.available_physical_bytes) << " available of "
                  << srm::core::format::bytes(mem.total_physical_bytes) << "\n";
    } else {
        std::cout << "Memory metrics unavailable: " << result.error().message << "\n";
    }
    return 0;
}
