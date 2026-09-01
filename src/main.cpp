#include <iostream>

#include <chrono>
#include <thread>

#include "core/format.hpp"
#include "platform/windows/cpu_monitor.hpp"
#include "platform/windows/disk_monitor.hpp"
#include "platform/windows/memory_monitor.hpp"

// Placeholder entry point wired directly to individual monitors as an
// end-to-end smoke check. Replaced by real CLI parsing, the monitor engine,
// and the presentation layer in later milestones.
int main() {
    srm::platform::windows::CpuMonitor cpu_monitor; // takes its baseline reading now
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    const auto cpu_result = cpu_monitor.sample();
    if (cpu_result) {
        std::cout << "CPU utilization: "
                  << srm::core::format::percent(cpu_result.value().total_utilization_percent) << "\n";
    } else {
        std::cout << "CPU metrics unavailable: " << cpu_result.error().message << "\n";
    }

    srm::platform::windows::MemoryMonitor memory_monitor;
    const auto mem_result = memory_monitor.sample();
    if (mem_result) {
        const auto& mem = mem_result.value();
        std::cout << "Physical memory: "
                  << srm::core::format::bytes(mem.available_physical_bytes) << " available of "
                  << srm::core::format::bytes(mem.total_physical_bytes) << "\n";
    } else {
        std::cout << "Memory metrics unavailable: " << mem_result.error().message << "\n";
    }

    srm::platform::windows::DiskMonitor disk_monitor;
    const auto disk_result = disk_monitor.sample();
    if (disk_result) {
        for (const auto& volume : disk_result.value()) {
            std::cout << volume.mount_point << " (" << volume.filesystem << "): "
                      << srm::core::format::bytes(volume.free_bytes) << " free of "
                      << srm::core::format::bytes(volume.total_bytes) << "\n";
        }
    } else {
        std::cout << "Disk metrics unavailable: " << disk_result.error().message << "\n";
    }

    return 0;
}
