#include "cpu_monitor.hpp"

#include <string>

#include "core/math.hpp"
#include "win_filetime.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace srm::platform::windows {

namespace {

RawCpuTimes read_raw_cpu_times() {
    RawCpuTimes raw;
    FILETIME idle{};
    FILETIME kernel{};
    FILETIME user{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        raw.succeeded = true;
        raw.idle_ticks = filetime_to_ticks(idle.dwLowDateTime, idle.dwHighDateTime);
        // Windows reports kernel time inclusive of idle time.
        raw.total_ticks = filetime_to_ticks(kernel.dwLowDateTime, kernel.dwHighDateTime) +
                           filetime_to_ticks(user.dwLowDateTime, user.dwHighDateTime);
    } else {
        raw.succeeded = false;
        raw.last_error = static_cast<std::uint32_t>(GetLastError());
    }
    return raw;
}

} // namespace

core::Result<core::CpuSnapshot> translate(const RawCpuTimes& previous, const RawCpuTimes& current) {
    if (!previous.succeeded) {
        return core::Result<core::CpuSnapshot>::Fail(core::Error{
            core::ErrorCode::Unavailable,
            "no prior successful CPU sample to compute utilization from",
        });
    }
    if (!current.succeeded) {
        return core::Result<core::CpuSnapshot>::Fail(core::Error{
            core::ErrorCode::PlatformApiFailure,
            "GetSystemTimes failed (GetLastError=" + std::to_string(current.last_error) + ")",
        });
    }

    core::CpuSnapshot snapshot;
    snapshot.total_utilization_percent = core::math::cpu_percent_from_ticks(
        previous.idle_ticks, previous.total_ticks, current.idle_ticks, current.total_ticks);
    // per_core_utilization_percent intentionally left empty -- see cpu_monitor.hpp.
    return core::Result<core::CpuSnapshot>::Ok(snapshot);
}

CpuMonitor::CpuMonitor() : previous_(read_raw_cpu_times()) {}

core::Result<core::CpuSnapshot> CpuMonitor::sample() {
    const RawCpuTimes current = read_raw_cpu_times();
    auto result = translate(previous_, current);
    if (current.succeeded) {
        // Refresh the baseline even if translate() failed on `previous`,
        // so a transient earlier failure doesn't permanently strand the
        // monitor with no way to recover a valid baseline.
        previous_ = current;
    }
    return result;
}

} // namespace srm::platform::windows
