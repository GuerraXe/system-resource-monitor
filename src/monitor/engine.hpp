#pragma once

// Orchestrates the six per-metric monitors behind platform/interfaces.hpp.
// Deliberately thin: per-metric interval bookkeeping (previous CPU ticks,
// previous process/interface byte counters) already lives inside each
// concrete monitor, so this class has nothing to add there. Its only job is
// bundling one sample() call per monitor into a single SystemSnapshot per
// poll(), with each field failing independently.
//
// Takes every monitor via dependency injection rather than constructing
// concrete Windows types itself, so it's unit testable with fake monitors
// and so the choice of which platform backend to instantiate stays in one
// place (main.cpp), not scattered into this class.
//
// Timing (the refresh-interval sleep loop) and shutdown signaling are
// deliberately NOT this class's job either -- poll() is a single,
// synchronous sample. The loop that calls it repeatedly lives in main.cpp,
// which is also where Ctrl+C handling belongs.

#include <memory>
#include <vector>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::monitor {

struct SystemSnapshot {
    core::Result<core::CpuSnapshot> cpu;
    core::Result<core::MemoryInfo> memory;
    core::Result<std::vector<core::DiskVolumeInfo>> disks;
    core::Result<std::vector<core::ProcessInfo>> processes;
    core::Result<std::vector<core::NetworkInterfaceInfo>> network;
    core::Result<core::SystemInfo> system;
};

class MonitorEngine {
public:
    MonitorEngine(std::unique_ptr<platform::ICpuMonitor> cpu, std::unique_ptr<platform::IMemoryMonitor> memory,
                  std::unique_ptr<platform::IDiskMonitor> disk, std::unique_ptr<platform::IProcessMonitor> process,
                  std::unique_ptr<platform::INetworkMonitor> network,
                  std::unique_ptr<platform::ISystemMonitor> system);

    SystemSnapshot poll();

private:
    std::unique_ptr<platform::ICpuMonitor> cpu_;
    std::unique_ptr<platform::IMemoryMonitor> memory_;
    std::unique_ptr<platform::IDiskMonitor> disk_;
    std::unique_ptr<platform::IProcessMonitor> process_;
    std::unique_ptr<platform::INetworkMonitor> network_;
    std::unique_ptr<platform::ISystemMonitor> system_;
};

} // namespace srm::monitor
