#include "network_monitor.hpp"

#include <string>
#include <utility>

#include "core/math.hpp"
#include "win_string.hpp"

// MIB_IF_TABLE2 and friends (netioapi.h, pulled in by iphlpapi.h) are
// gated on _WS2IPDEF_ (defined by ws2ipdef.h, pulled in by ws2tcpip.h)
// having already been seen -- without it that whole region of netioapi.h
// is silently skipped by the preprocessor. WIN32_LEAN_AND_MEAN keeps
// windows.h from separately pulling in the old winsock.h, which would
// otherwise conflict with winsock2.h/ws2tcpip.h.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

#include <iphlpapi.h>

namespace srm::platform::windows {

namespace {
// IANA ifType-MIB value for the software loopback interface (not exposed
// as a named constant in every SDK version, so it's spelled out here).
constexpr ULONG kLoopbackIfType = 24;
} // namespace

std::vector<core::NetworkInterfaceInfo> translate(
    const std::vector<RawInterfaceSample>& raw,
    const std::unordered_map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>>& previous_bytes,
    double elapsed_seconds) {
    std::vector<core::NetworkInterfaceInfo> result;
    result.reserve(raw.size());

    for (const auto& r : raw) {
        if (!r.operational || r.loopback || r.filter_interface) {
            continue;
        }

        core::NetworkInterfaceInfo info;
        info.name = r.name;

        if (const auto it = previous_bytes.find(r.luid); it != previous_bytes.end()) {
            info.receive_bytes_per_second =
                core::math::rate_per_second(it->second.first, r.received_bytes, elapsed_seconds);
            info.send_bytes_per_second =
                core::math::rate_per_second(it->second.second, r.sent_bytes, elapsed_seconds);
        }
        // else: no prior reading for this interface -- rates stay 0.0.

        result.push_back(std::move(info));
    }

    return result;
}

NetworkMonitor::NetworkMonitor() : previous_sample_time_(std::chrono::steady_clock::now()) {}

core::Result<std::vector<core::NetworkInterfaceInfo>> NetworkMonitor::sample() {
    MIB_IF_TABLE2* table = nullptr;
    const auto status = GetIfTable2(&table);
    if (status != NO_ERROR) {
        return core::Result<std::vector<core::NetworkInterfaceInfo>>::Fail(core::Error{
            core::ErrorCode::PlatformApiFailure,
            "GetIfTable2 failed (error=" + std::to_string(status) + ")",
        });
    }

    std::vector<RawInterfaceSample> raw;
    raw.reserve(table->NumEntries);
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2& row = table->Table[i];
        RawInterfaceSample sample;
        sample.luid = row.InterfaceLuid.Value;
        sample.name = narrow_or_empty(row.Alias);
        sample.operational = (row.OperStatus == IfOperStatusUp);
        sample.loopback = (row.Type == kLoopbackIfType);
        sample.filter_interface = static_cast<bool>(row.InterfaceAndOperStatusFlags.FilterInterface);
        sample.received_bytes = row.InOctets;
        sample.sent_bytes = row.OutOctets;
        raw.push_back(std::move(sample));
    }
    FreeMibTable(table);

    const auto now = std::chrono::steady_clock::now();
    const double elapsed_seconds = std::chrono::duration<double>(now - previous_sample_time_).count();

    auto interfaces = translate(raw, previous_bytes_, elapsed_seconds);

    previous_bytes_.clear();
    for (const auto& r : raw) {
        previous_bytes_.emplace(r.luid, std::make_pair(r.received_bytes, r.sent_bytes));
    }
    previous_sample_time_ = now;

    return core::Result<std::vector<core::NetworkInterfaceInfo>>::Ok(std::move(interfaces));
}

} // namespace srm::platform::windows
