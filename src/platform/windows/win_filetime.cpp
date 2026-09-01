#include "win_filetime.hpp"

namespace srm::platform::windows {

std::uint64_t filetime_to_ticks(std::uint32_t low, std::uint32_t high) noexcept {
    return (static_cast<std::uint64_t>(high) << 32) | low;
}

} // namespace srm::platform::windows
