#pragma once

// Plain data types shared by every platform backend and the presentation
// layer. Deliberately free of any OS-specific types (HANDLE, DWORD, ...) so
// this header compiles and means the same thing regardless of which
// platform/ backend produced the values.

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace srm::core {

// Instantaneous: read directly from a single OS query, no history needed.
struct MemoryInfo {
    std::uint64_t total_physical_bytes = 0;
    std::uint64_t available_physical_bytes = 0;
};

// Instantaneous.
struct DiskVolumeInfo {
    std::string mount_point;  // e.g. "C:\\"
    std::string filesystem;   // e.g. "NTFS"; empty if the OS couldn't report one
    std::uint64_t total_bytes = 0;
    std::uint64_t free_bytes = 0;
};

// Interval-based: utilization only means something as a delta between two
// counter samples taken `interval` apart, so a fresh CpuMonitor needs at
// least one throwaway sample before its numbers are meaningful.
struct CpuSnapshot {
    double total_utilization_percent = 0.0;                // 0..100, all cores combined
    std::vector<double> per_core_utilization_percent;       // 0..100 each, index == core id
};

// Interval-based: cpu_percent is this process's CPU time consumed during
// the last sampling interval, not a cumulative total.
struct ProcessInfo {
    std::uint32_t pid = 0;
    std::string name;
    double cpu_percent = 0.0;
    std::uint64_t working_set_bytes = 0;
};

// Interval-based: throughput, not cumulative byte counters.
struct NetworkInterfaceInfo {
    std::string name;
    double receive_bytes_per_second = 0.0;
    double send_bytes_per_second = 0.0;
};

// Instantaneous.
struct SystemInfo {
    std::string hostname;
    std::chrono::seconds uptime{0};
};

} // namespace srm::core
