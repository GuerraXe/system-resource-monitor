#pragma once

// Windows backend for IProcessMonitor: enumerates via
// CreateToolhelp32Snapshot, then for each pid opens a minimal-rights handle
// (PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ) to read CPU time
// (GetProcessTimes) and working-set memory (GetProcessMemoryInfo).
//
// Some processes (the swapper/idle process, protected system processes,
// anything owned by another user without elevation) can't be opened at that
// access level even by an administrator. Those still appear in the result
// with their pid and name -- Toolhelp32 doesn't need a handle for that --
// just with cpu_percent and working_set_bytes left at 0, the same value a
// genuinely idle process with no resident memory would show. This project
// doesn't distinguish "measured zero" from "couldn't measure" per field;
// see ARCHITECTURE.md's design-decisions section.

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::platform::windows {

struct RawProcessSample {
    std::uint32_t pid = 0;
    std::string name;
    bool cpu_query_succeeded = false;
    std::uint64_t cpu_ticks = 0; // cumulative kernel+user time since process start, 100ns units
    bool memory_query_succeeded = false;
    std::uint64_t working_set_bytes = 0;
};

// Pure: builds one ProcessInfo per raw entry. cpu_percent comes from
// core::math::cpu_percent_of_wall_time against `previous_ticks` (looked up
// by pid; absent means "no prior reading," which yields 0%, not an error).
// No Win32 types involved, so this is exercised directly with synthetic
// samples.
std::vector<core::ProcessInfo> translate(const std::vector<RawProcessSample>& raw,
                                          const std::unordered_map<std::uint32_t, std::uint64_t>& previous_ticks,
                                          double elapsed_seconds);

class ProcessMonitor final : public IProcessMonitor {
public:
    ProcessMonitor();
    core::Result<std::vector<core::ProcessInfo>> sample() override;

private:
    std::unordered_map<std::uint32_t, std::uint64_t> previous_ticks_;
    std::chrono::steady_clock::time_point previous_sample_time_;
};

} // namespace srm::platform::windows
