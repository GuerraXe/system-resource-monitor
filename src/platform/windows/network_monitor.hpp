#pragma once

// Windows backend for INetworkMonitor, built on the IP Helper API's
// GetIfTable2. Loopback, operationally-down, and *filter* interfaces are
// excluded (in translate(), so the filtering logic itself is unit tested).
// "Filter" interfaces are NDIS lightweight-filter driver bindings (WFP,
// QoS Packet Scheduler, ...) that GetIfTable2 reports as their own rows,
// each mirroring the exact same traffic as the physical adapter underneath
// -- without excluding them, a single Wi-Fi or Ethernet adapter shows up as
// five or six near-duplicate rows with identical throughput.

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/result.hpp"
#include "core/snapshot.hpp"
#include "platform/interfaces.hpp"

namespace srm::platform::windows {

struct RawInterfaceSample {
    std::uint64_t luid = 0; // NET_LUID.Value: stable identity across calls, unlike an index
    std::string name;
    bool operational = false; // true if OperStatus == IfOperStatusUp
    bool loopback = false;
    bool filter_interface = false; // NDIS LWF binding shadowing another adapter's traffic
    std::uint64_t received_bytes = 0; // cumulative since the interface came up
    std::uint64_t sent_bytes = 0;
};

// previous_bytes maps luid -> (received_bytes, sent_bytes) from the last
// sample(). Drops non-operational and loopback interfaces; for the rest,
// computes throughput via core::math::rate_per_second (0 if this luid has
// no entry in previous_bytes). No Win32 types involved.
std::vector<core::NetworkInterfaceInfo> translate(
    const std::vector<RawInterfaceSample>& raw,
    const std::unordered_map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>>& previous_bytes,
    double elapsed_seconds);

class NetworkMonitor final : public INetworkMonitor {
public:
    NetworkMonitor();
    core::Result<std::vector<core::NetworkInterfaceInfo>> sample() override;

private:
    std::unordered_map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>> previous_bytes_;
    std::chrono::steady_clock::time_point previous_sample_time_;
};

} // namespace srm::platform::windows
