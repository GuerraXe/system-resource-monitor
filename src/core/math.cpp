#include "math.hpp"

#include <algorithm>

namespace srm::core::math {

double percent_used(std::uint64_t total, std::uint64_t available) noexcept {
    if (total == 0) {
        return 0.0;
    }
    const std::uint64_t used = available >= total ? 0 : total - available;
    const double percent = 100.0 * static_cast<double>(used) / static_cast<double>(total);
    return std::clamp(percent, 0.0, 100.0);
}

double rate_per_second(std::uint64_t previous, std::uint64_t current, double elapsed_seconds) noexcept {
    if (elapsed_seconds <= 0.0 || current < previous) {
        return 0.0;
    }
    return static_cast<double>(current - previous) / elapsed_seconds;
}

double cpu_percent_from_ticks(std::uint64_t prev_idle, std::uint64_t prev_total,
                               std::uint64_t curr_idle, std::uint64_t curr_total) noexcept {
    if (curr_total <= prev_total) {
        return 0.0;
    }
    const std::uint64_t total_delta = curr_total - prev_total;
    const std::uint64_t idle_delta = curr_idle >= prev_idle
        ? std::min(curr_idle - prev_idle, total_delta)
        : 0;
    const std::uint64_t busy_delta = total_delta - idle_delta;
    const double percent = 100.0 * static_cast<double>(busy_delta) / static_cast<double>(total_delta);
    return std::clamp(percent, 0.0, 100.0);
}

} // namespace srm::core::math
