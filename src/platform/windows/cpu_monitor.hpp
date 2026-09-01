#pragma once

// Windows backend for ICpuMonitor, built on GetSystemTimes rather than a
// PDH counter. GetSystemTimes reports cumulative idle/kernel/user time
// across all logical processors since boot in 100ns units; on Windows,
// kernel time already includes idle time, so:
//   total_ticks = kernel_time + user_time
//   idle_ticks  = idle_time
// is exactly the (idle, total) tick-pair model core::math::cpu_percent_from_ticks
// expects, and it normalizes correctly regardless of core count without
// needing to know how many cores exist. This also mirrors /proc/stat's
// idle/total split closely enough that a future Linux backend reuses the
// same math with no changes.
//
// Per-core utilization is intentionally out of scope: it requires the
// undocumented NtQuerySystemInformation(SystemProcessorPerformanceInformation)
// call, which is a materially bigger and less stable surface than this
// project's other Win32 usage. CpuSnapshot::per_core_utilization_percent is
// always empty from this backend; see ARCHITECTURE.md.

#include <cstdint>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::platform::windows {

struct RawCpuTimes {
    bool succeeded = false;
    std::uint32_t last_error = 0;
    std::uint64_t idle_ticks = 0;
    std::uint64_t total_ticks = 0;
};

// Pure: computes the interval's CPU% from two raw readings. Requires both
// `previous` and `current` to have succeeded; if either didn't, returns a
// Result::Fail explaining which. No Win32 types involved, so this is
// exercised directly by tests with synthetic readings.
core::Result<core::CpuSnapshot> translate(const RawCpuTimes& previous, const RawCpuTimes& current);

class CpuMonitor final : public ICpuMonitor {
public:
    CpuMonitor();
    core::Result<core::CpuSnapshot> sample() override;

private:
    RawCpuTimes previous_;
};

} // namespace srm::platform::windows
