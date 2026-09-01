#pragma once

// Trivial test doubles for platform/interfaces.hpp: each returns a fixed,
// caller-supplied Result from every sample() call. This is what lets
// MonitorEngine's orchestration logic be verified without touching real
// hardware -- these fakes have no OS dependency at all.

#include <utility>

#include "platform/interfaces.hpp"

namespace srm::monitor::test {

template <typename Interface, typename Value>
class FakeMonitor final : public Interface {
public:
    explicit FakeMonitor(core::Result<Value> result) : result_(std::move(result)) {}
    core::Result<Value> sample() override { return result_; }

private:
    core::Result<Value> result_;
};

using FakeCpuMonitor = FakeMonitor<platform::ICpuMonitor, core::CpuSnapshot>;
using FakeMemoryMonitor = FakeMonitor<platform::IMemoryMonitor, core::MemoryInfo>;
using FakeDiskMonitor = FakeMonitor<platform::IDiskMonitor, std::vector<core::DiskVolumeInfo>>;
using FakeProcessMonitor = FakeMonitor<platform::IProcessMonitor, std::vector<core::ProcessInfo>>;
using FakeNetworkMonitor = FakeMonitor<platform::INetworkMonitor, std::vector<core::NetworkInterfaceInfo>>;
using FakeSystemMonitor = FakeMonitor<platform::ISystemMonitor, core::SystemInfo>;

} // namespace srm::monitor::test
