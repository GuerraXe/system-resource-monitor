#pragma once

// Pure, OS-independent math extracted out of the monitors so it can be unit
// tested with synthetic inputs instead of requiring real hardware counters.

#include <cstdint>

namespace srm::core::math {

// Percentage of `total` currently in use, given how much is free/available.
// Returns 0.0 if total == 0 (a zero-capacity resource is 0% used, not an
// error). Result is clamped to [0, 100] to absorb tiny inconsistencies in
// what the OS reports for total vs. available.
double percent_used(std::uint64_t total, std::uint64_t available) noexcept;

// Rate (units of the counter per second) from two cumulative counter
// readings taken elapsed_seconds apart. Returns 0.0 if elapsed_seconds <= 0,
// or if current < previous (the counter reset or wrapped, e.g. a NIC driver
// reload) rather than producing a nonsensical negative rate.
double rate_per_second(std::uint64_t previous, std::uint64_t current, double elapsed_seconds) noexcept;

// CPU utilization percent derived from two (idle, total) tick-count pairs
// sampled at the start and end of an interval. Both pairs must use the same
// tick unit (100ns PDH intervals, jiffies, etc.); elapsed wall-clock time is
// implicit in the tick deltas so it isn't a separate parameter. Returns 0.0
// for a non-advancing or wrapped total-tick counter.
double cpu_percent_from_ticks(std::uint64_t prev_idle, std::uint64_t prev_total,
                               std::uint64_t curr_idle, std::uint64_t curr_total) noexcept;

} // namespace srm::core::math
