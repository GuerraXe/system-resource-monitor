#pragma once

// Windows backend for IMemoryMonitor. Deliberately split into two pieces:
//
//   - RawMemoryStatus + translate(): plain data in, Result<MemoryInfo> out,
//     zero Win32 types involved. This is the actual logic (success/failure
//     branching, field mapping) and is unit tested directly by constructing
//     synthetic RawMemoryStatus values -- including failure cases that
//     can't be reliably provoked by calling the real Windows API from a
//     test.
//   - MemoryMonitor::sample(): the thin, effectively untestable-in-isolation
//     wrapper that calls GlobalMemoryStatusEx and hands its output to
//     translate(). Covered by an integration/invariant test instead.
//
// <windows.h> is included only in memory_monitor.cpp, never here, so
// anything that depends on this header (including tests/) is not forced to
// pull in the Win32 SDK headers.

#include <cstdint>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::platform::windows {

// Mirrors the fields of Win32's MEMORYSTATUSEX that we care about, plus a
// success flag and the raw GetLastError() code so translate() can build a
// meaningful error message without ever seeing a Win32 type.
struct RawMemoryStatus {
    bool succeeded = false;
    std::uint32_t last_error = 0;
    std::uint64_t total_physical_bytes = 0;
    std::uint64_t available_physical_bytes = 0;
};

core::Result<core::MemoryInfo> translate(const RawMemoryStatus& raw);

class MemoryMonitor final : public IMemoryMonitor {
public:
    core::Result<core::MemoryInfo> sample() override;
};

} // namespace srm::platform::windows
