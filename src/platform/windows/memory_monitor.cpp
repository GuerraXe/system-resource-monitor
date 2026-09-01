#include "memory_monitor.hpp"

#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace srm::platform::windows {

core::Result<core::MemoryInfo> translate(const RawMemoryStatus& raw) {
    if (!raw.succeeded) {
        return core::Result<core::MemoryInfo>::Fail(core::Error{
            core::ErrorCode::PlatformApiFailure,
            "GlobalMemoryStatusEx failed (GetLastError=" + std::to_string(raw.last_error) + ")",
        });
    }

    core::MemoryInfo info;
    info.total_physical_bytes = raw.total_physical_bytes;
    info.available_physical_bytes = raw.available_physical_bytes;
    info.total_page_file_bytes = raw.total_page_file_bytes;
    info.available_page_file_bytes = raw.available_page_file_bytes;
    return core::Result<core::MemoryInfo>::Ok(info);
}

core::Result<core::MemoryInfo> MemoryMonitor::sample() {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);

    RawMemoryStatus raw;
    if (GlobalMemoryStatusEx(&status)) {
        raw.succeeded = true;
        raw.total_physical_bytes = status.ullTotalPhys;
        raw.available_physical_bytes = status.ullAvailPhys;
        raw.total_page_file_bytes = status.ullTotalPageFile;
        raw.available_page_file_bytes = status.ullAvailPageFile;
    } else {
        raw.succeeded = false;
        raw.last_error = static_cast<std::uint32_t>(GetLastError());
    }

    return translate(raw);
}

} // namespace srm::platform::windows
