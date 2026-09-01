#include "process_monitor.hpp"

#include <string>
#include <utility>

#include "core/math.hpp"
#include "win_filetime.hpp"
#include "win_string.hpp"

// windows.h must come first: psapi.h and tlhelp32.h both assume its types
// (BOOL, DWORD, WINAPI, ...) are already defined and don't include it
// themselves.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <psapi.h>
#include <tlhelp32.h>

namespace srm::platform::windows {

namespace {
constexpr double kFiletimeTicksPerSecond = 10'000'000.0;
}

std::vector<core::ProcessInfo> translate(const std::vector<RawProcessSample>& raw,
                                          const std::unordered_map<std::uint32_t, std::uint64_t>& previous_ticks,
                                          double elapsed_seconds) {
    std::vector<core::ProcessInfo> result;
    result.reserve(raw.size());

    for (const auto& r : raw) {
        core::ProcessInfo info;
        info.pid = r.pid;
        info.name = r.name;

        if (r.cpu_query_succeeded) {
            if (const auto it = previous_ticks.find(r.pid); it != previous_ticks.end()) {
                info.cpu_percent = core::math::cpu_percent_of_wall_time(it->second, r.cpu_ticks, elapsed_seconds,
                                                                          kFiletimeTicksPerSecond);
            }
            // else: no prior reading for this pid (new process, or first
            // sample() call ever) -- cpu_percent stays at its 0.0 default.
        }

        if (r.memory_query_succeeded) {
            info.working_set_bytes = r.working_set_bytes;
        }

        result.push_back(std::move(info));
    }

    return result;
}

ProcessMonitor::ProcessMonitor() : previous_sample_time_(std::chrono::steady_clock::now()) {}

core::Result<std::vector<core::ProcessInfo>> ProcessMonitor::sample() {
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return core::Result<std::vector<core::ProcessInfo>>::Fail(core::Error{
            core::ErrorCode::PlatformApiFailure,
            "CreateToolhelp32Snapshot failed (GetLastError=" + std::to_string(GetLastError()) + ")",
        });
    }

    std::vector<RawProcessSample> raw;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            RawProcessSample sample;
            sample.pid = entry.th32ProcessID;
            sample.name = narrow_or_empty(entry.szExeFile);

            // PROCESS_QUERY_LIMITED_INFORMATION is the minimal right that
            // still permits GetProcessTimes/GetProcessMemoryInfo; several
            // system/protected processes deny even this, which is why
            // cpu_query_succeeded / memory_query_succeeded exist as
            // independent flags rather than treating OpenProcess failure
            // as a reason to drop the process entirely.
            if (const HANDLE process =
                    OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
                process != nullptr) {
                FILETIME creation_time{};
                FILETIME exit_time{};
                FILETIME kernel_time{};
                FILETIME user_time{};
                if (GetProcessTimes(process, &creation_time, &exit_time, &kernel_time, &user_time)) {
                    sample.cpu_query_succeeded = true;
                    sample.cpu_ticks = filetime_to_ticks(kernel_time.dwLowDateTime, kernel_time.dwHighDateTime) +
                                        filetime_to_ticks(user_time.dwLowDateTime, user_time.dwHighDateTime);
                }

                PROCESS_MEMORY_COUNTERS counters{};
                counters.cb = sizeof(counters);
                if (GetProcessMemoryInfo(process, &counters, sizeof(counters))) {
                    sample.memory_query_succeeded = true;
                    sample.working_set_bytes = counters.WorkingSetSize;
                }

                CloseHandle(process);
            }

            raw.push_back(std::move(sample));
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    const auto now = std::chrono::steady_clock::now();
    const double elapsed_seconds = std::chrono::duration<double>(now - previous_sample_time_).count();

    auto processes = translate(raw, previous_ticks_, elapsed_seconds);

    previous_ticks_.clear();
    for (const auto& r : raw) {
        if (r.cpu_query_succeeded) {
            previous_ticks_.emplace(r.pid, r.cpu_ticks);
        }
    }
    previous_sample_time_ = now;

    return core::Result<std::vector<core::ProcessInfo>>::Ok(std::move(processes));
}

} // namespace srm::platform::windows
