#include "disk_monitor.hpp"

#include <iterator>
#include <string>

#include "win_string.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace srm::platform::windows {

std::vector<core::DiskVolumeInfo> translate(const std::vector<RawVolumeQuery>& raw) {
    std::vector<core::DiskVolumeInfo> volumes;
    volumes.reserve(raw.size());
    for (const auto& q : raw) {
        if (!q.space_query_succeeded) {
            continue;
        }
        core::DiskVolumeInfo info;
        info.mount_point = q.mount_point;
        info.filesystem = q.filesystem;
        info.total_bytes = q.total_bytes;
        info.free_bytes = q.free_bytes;
        volumes.push_back(std::move(info));
    }
    return volumes;
}

core::Result<std::vector<core::DiskVolumeInfo>> DiskMonitor::sample() {
    const DWORD drive_mask = GetLogicalDrives();
    if (drive_mask == 0) {
        return core::Result<std::vector<core::DiskVolumeInfo>>::Fail(core::Error{
            core::ErrorCode::PlatformApiFailure,
            "GetLogicalDrives failed (GetLastError=" + std::to_string(GetLastError()) + ")",
        });
    }

    std::vector<RawVolumeQuery> raw;
    for (int letter = 0; letter < 26; ++letter) {
        if ((drive_mask & (1u << letter)) == 0) {
            continue;
        }

        const std::wstring root = std::wstring(1, static_cast<wchar_t>(L'A' + letter)) + L":\\";

        RawVolumeQuery q;
        q.mount_point = std::string(1, static_cast<char>('A' + letter)) + ":\\";

        ULARGE_INTEGER free_available{};
        ULARGE_INTEGER total_bytes{};
        ULARGE_INTEGER total_free{};
        if (GetDiskFreeSpaceExW(root.c_str(), &free_available, &total_bytes, &total_free)) {
            q.space_query_succeeded = true;
            q.total_bytes = total_bytes.QuadPart;
            q.free_bytes = total_free.QuadPart;
        }

        wchar_t fs_name[MAX_PATH + 1] = {};
        if (GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fs_name,
                                   static_cast<DWORD>(std::size(fs_name)))) {
            q.filesystem = narrow_or_empty(fs_name);
        }

        raw.push_back(std::move(q));
    }

    return core::Result<std::vector<core::DiskVolumeInfo>>::Ok(translate(raw));
}

} // namespace srm::platform::windows
