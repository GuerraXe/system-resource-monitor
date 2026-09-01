#pragma once

// Windows backend for IDiskMonitor. Same split as memory_monitor.hpp: a
// pure translate() doing the real logic (here, filtering out volumes whose
// space query failed) versus a thin sample() that talks to Win32.

#include <cstdint>
#include <string>
#include <vector>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::platform::windows {

struct RawVolumeQuery {
    std::string mount_point;
    bool space_query_succeeded = false;
    std::uint64_t total_bytes = 0;
    std::uint64_t free_bytes = 0;
    std::string filesystem; // empty if unknown (name query failed or wasn't attempted)
};

// Drops entries where the space query failed; maps the rest 1:1 to
// DiskVolumeInfo. Order is preserved.
std::vector<core::DiskVolumeInfo> translate(const std::vector<RawVolumeQuery>& raw);

class DiskMonitor final : public IDiskMonitor {
public:
    core::Result<std::vector<core::DiskVolumeInfo>> sample() override;
};

} // namespace srm::platform::windows
