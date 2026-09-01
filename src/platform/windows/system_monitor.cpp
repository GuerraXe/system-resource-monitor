#include "system_monitor.hpp"

#include <iterator>

#include "win_string.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace srm::platform::windows {

core::Result<core::SystemInfo> translate(const RawSystemInfo& raw) {
    core::SystemInfo info;
    info.hostname = raw.hostname_query_succeeded ? raw.hostname : std::string{};
    info.uptime = std::chrono::seconds(raw.uptime_millis / 1000);
    return core::Result<core::SystemInfo>::Ok(info);
}

core::Result<core::SystemInfo> SystemMonitor::sample() {
    RawSystemInfo raw;
    raw.uptime_millis = GetTickCount64();

    wchar_t name_buffer[256] = {};
    DWORD size = static_cast<DWORD>(std::size(name_buffer));
    if (GetComputerNameExW(ComputerNamePhysicalDnsHostname, name_buffer, &size)) {
        raw.hostname_query_succeeded = true;
        raw.hostname = narrow_or_empty(name_buffer);
    }

    return translate(raw);
}

} // namespace srm::platform::windows
