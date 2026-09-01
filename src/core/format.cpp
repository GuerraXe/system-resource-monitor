#include "format.hpp"

#include <array>
#include <iomanip>
#include <sstream>

namespace srm::core::format {

std::string bytes(std::uint64_t value) {
    static constexpr std::array<const char*, 6> units{"B", "KB", "MB", "GB", "TB", "PB"};

    double scaled = static_cast<double>(value);
    std::size_t unit_index = 0;
    while (scaled >= 1024.0 && unit_index + 1 < units.size()) {
        scaled /= 1024.0;
        ++unit_index;
    }

    std::ostringstream oss;
    if (unit_index == 0) {
        oss << value << ' ' << units[unit_index];
    } else {
        oss << std::fixed << std::setprecision(1) << scaled << ' ' << units[unit_index];
    }
    return oss.str();
}

std::string percent(double value, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value << '%';
    return oss.str();
}

std::string duration_seconds(std::uint64_t total_seconds) {
    const std::uint64_t days = total_seconds / 86400;
    const std::uint64_t hours = (total_seconds % 86400) / 3600;
    const std::uint64_t minutes = (total_seconds % 3600) / 60;
    const std::uint64_t seconds = total_seconds % 60;

    std::ostringstream oss;
    bool started = false;
    if (days > 0) {
        oss << days << "d ";
        started = true;
    }
    if (started || hours > 0) {
        oss << hours << "h ";
        started = true;
    }
    if (started || minutes > 0) {
        oss << minutes << "m ";
        started = true;
    }
    oss << seconds << 's';
    return oss.str();
}

} // namespace srm::core::format
