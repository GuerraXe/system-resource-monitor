#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

#include "core/format.hpp"
#include "monitor/engine.hpp"
#include "platform/windows/cpu_monitor.hpp"
#include "platform/windows/disk_monitor.hpp"
#include "platform/windows/memory_monitor.hpp"
#include "platform/windows/network_monitor.hpp"
#include "platform/windows/process_monitor.hpp"
#include "platform/windows/system_monitor.hpp"

namespace {

void print_snapshot(const srm::monitor::SystemSnapshot& snapshot) {
    if (snapshot.system) {
        const auto& info = snapshot.system.value();
        std::cout << info.hostname << " up "
                  << srm::core::format::duration_seconds(static_cast<std::uint64_t>(info.uptime.count())) << "\n";
    } else {
        std::cout << "System metrics unavailable: " << snapshot.system.error().message << "\n";
    }

    if (snapshot.cpu) {
        std::cout << "CPU utilization: "
                  << srm::core::format::percent(snapshot.cpu.value().total_utilization_percent) << "\n";
    } else {
        std::cout << "CPU metrics unavailable: " << snapshot.cpu.error().message << "\n";
    }

    if (snapshot.memory) {
        const auto& mem = snapshot.memory.value();
        std::cout << "Physical memory: "
                  << srm::core::format::bytes(mem.available_physical_bytes) << " available of "
                  << srm::core::format::bytes(mem.total_physical_bytes) << "\n";
    } else {
        std::cout << "Memory metrics unavailable: " << snapshot.memory.error().message << "\n";
    }

    if (snapshot.disks) {
        for (const auto& volume : snapshot.disks.value()) {
            std::cout << volume.mount_point << " (" << volume.filesystem << "): "
                      << srm::core::format::bytes(volume.free_bytes) << " free of "
                      << srm::core::format::bytes(volume.total_bytes) << "\n";
        }
    } else {
        std::cout << "Disk metrics unavailable: " << snapshot.disks.error().message << "\n";
    }

    if (snapshot.processes) {
        auto processes = snapshot.processes.value();
        std::cout << processes.size() << " processes; top 5 by CPU:\n";
        std::sort(processes.begin(), processes.end(),
                  [](const auto& a, const auto& b) { return a.cpu_percent > b.cpu_percent; });
        for (std::size_t i = 0; i < processes.size() && i < 5; ++i) {
            const auto& p = processes[i];
            std::cout << "  " << p.pid << "  " << p.name << "  " << srm::core::format::percent(p.cpu_percent)
                      << "  " << srm::core::format::bytes(p.working_set_bytes) << "\n";
        }
    } else {
        std::cout << "Process metrics unavailable: " << snapshot.processes.error().message << "\n";
    }

    if (snapshot.network) {
        for (const auto& iface : snapshot.network.value()) {
            std::cout << iface.name << ": "
                      << srm::core::format::bytes(static_cast<std::uint64_t>(iface.receive_bytes_per_second))
                      << "/s down, "
                      << srm::core::format::bytes(static_cast<std::uint64_t>(iface.send_bytes_per_second))
                      << "/s up\n";
        }
    } else {
        std::cout << "Network metrics unavailable: " << snapshot.network.error().message << "\n";
    }
}

} // namespace

// Placeholder entry point: constructs the real Windows backends, wires them
// into one MonitorEngine, and prints one snapshot. Replaced by real CLI
// parsing, a proper refresh loop, and the presentation layer in later
// milestones.
int main() {
    using namespace srm::platform::windows;

    srm::monitor::MonitorEngine engine(
        std::make_unique<CpuMonitor>(), std::make_unique<MemoryMonitor>(), std::make_unique<DiskMonitor>(),
        std::make_unique<ProcessMonitor>(), std::make_unique<NetworkMonitor>(), std::make_unique<SystemMonitor>());

    engine.poll(); // throwaway: primes the interval-based monitors' baselines
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    print_snapshot(engine.poll());

    return 0;
}
