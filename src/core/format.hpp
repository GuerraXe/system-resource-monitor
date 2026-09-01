#pragma once

// Human-readable text formatting for the presentation layer, kept separate
// and pure (no I/O) so it's unit testable without a terminal.

#include <cstdint>
#include <string>

namespace srm::core::format {

// e.g. 1536 -> "1.5 KB", 0 -> "0 B". Uses 1024-based (KiB/MiB/...) units but
// the conventional single-letter suffixes (KB/MB/GB/TB) to match what most
// system monitoring tools show.
std::string bytes(std::uint64_t value);

// e.g. 42.5 -> "42.5%". `decimals` controls precision; percent is not
// clamped here since callers (core::math) are responsible for producing
// already-valid [0, 100] values -- formatting shouldn't silently hide a
// logic bug elsewhere by clamping it away.
std::string percent(double value, int decimals = 1);

// e.g. 3725s -> "1h 2m 5s"; 45s -> "45s". Omits leading zero-value units.
std::string duration_seconds(std::uint64_t total_seconds);

} // namespace srm::core::format
