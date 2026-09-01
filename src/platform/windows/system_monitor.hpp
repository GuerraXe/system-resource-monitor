#pragma once

// Windows backend for ISystemMonitor: GetTickCount64 for uptime (monotonic
// milliseconds since boot; at 64 bits it doesn't wrap around on any
// realistic uptime) and GetComputerNameExW for the hostname.

#include <cstdint>
#include <string>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::platform::windows {

struct RawSystemInfo {
    bool hostname_query_succeeded = false;
    std::string hostname;
    std::uint64_t uptime_millis = 0;
};

// Pure: hostname stays empty if the query didn't succeed, but this never
// fails outright -- see the ISystemMonitor doc comment for why.
core::Result<core::SystemInfo> translate(const RawSystemInfo& raw);

class SystemMonitor final : public ISystemMonitor {
public:
    core::Result<core::SystemInfo> sample() override;
};

} // namespace srm::platform::windows
