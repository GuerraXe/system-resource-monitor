#pragma once

// Shared helper for the two backends (cpu_monitor, process_monitor) that
// read Win32 FILETIME-based tick counters. Takes the struct's two 32-bit
// fields as plain integers rather than a FILETIME reference, so this header
// needs no <windows.h> dependency at all -- callers pass
// filetime_to_ticks(ft.dwLowDateTime, ft.dwHighDateTime).

#include <cstdint>

namespace srm::platform::windows {

// Combines a FILETIME's low/high 32-bit halves into a single 100ns tick
// count (the same layout FILETIME uses: high 32 bits shifted above low).
std::uint64_t filetime_to_ticks(std::uint32_t low, std::uint32_t high) noexcept;

} // namespace srm::platform::windows
