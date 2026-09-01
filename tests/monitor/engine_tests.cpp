#include "monitor/engine.hpp"

#include "fake_monitors.hpp"
#include "support/test_framework.hpp"

namespace core = srm::core;
using namespace srm::monitor::test;
using srm::core::Error;
using srm::core::ErrorCode;
using srm::core::Result;
using srm::monitor::MonitorEngine;

TEST_CASE("engine: poll assembles one field per monitor from all-success inputs") {
    core::CpuSnapshot cpu;
    cpu.total_utilization_percent = 42.0;
    core::MemoryInfo mem;
    mem.total_physical_bytes = 1000;
    core::SystemInfo sys;
    sys.hostname = "test-host";

    MonitorEngine engine(std::make_unique<FakeCpuMonitor>(Result<core::CpuSnapshot>::Ok(cpu)),
                          std::make_unique<FakeMemoryMonitor>(Result<core::MemoryInfo>::Ok(mem)),
                          std::make_unique<FakeDiskMonitor>(
                              Result<std::vector<core::DiskVolumeInfo>>::Ok({})),
                          std::make_unique<FakeProcessMonitor>(
                              Result<std::vector<core::ProcessInfo>>::Ok({})),
                          std::make_unique<FakeNetworkMonitor>(
                              Result<std::vector<core::NetworkInterfaceInfo>>::Ok({})),
                          std::make_unique<FakeSystemMonitor>(Result<core::SystemInfo>::Ok(sys)));

    auto snapshot = engine.poll();

    CHECK(static_cast<bool>(snapshot.cpu));
    CHECK(snapshot.cpu.value().total_utilization_percent == 42.0);
    CHECK(static_cast<bool>(snapshot.memory));
    CHECK_EQ(snapshot.memory.value().total_physical_bytes, static_cast<std::uint64_t>(1000));
    CHECK(static_cast<bool>(snapshot.system));
    CHECK_EQ(snapshot.system.value().hostname, std::string("test-host"));
}

TEST_CASE("engine: one monitor failing doesn't prevent the others from reporting") {
    core::MemoryInfo mem;
    mem.total_physical_bytes = 2000;

    MonitorEngine engine(
        std::make_unique<FakeCpuMonitor>(
            Result<core::CpuSnapshot>::Fail(Error{ErrorCode::Unavailable, "cpu counter missing"})),
        std::make_unique<FakeMemoryMonitor>(Result<core::MemoryInfo>::Ok(mem)),
        std::make_unique<FakeDiskMonitor>(Result<std::vector<core::DiskVolumeInfo>>::Ok({})),
        std::make_unique<FakeProcessMonitor>(Result<std::vector<core::ProcessInfo>>::Ok({})),
        std::make_unique<FakeNetworkMonitor>(Result<std::vector<core::NetworkInterfaceInfo>>::Ok({})),
        std::make_unique<FakeSystemMonitor>(Result<core::SystemInfo>::Ok(core::SystemInfo{})));

    auto snapshot = engine.poll();

    CHECK(!static_cast<bool>(snapshot.cpu));
    CHECK(snapshot.cpu.error().code == ErrorCode::Unavailable);
    // Memory sampling is entirely independent of the CPU monitor failing.
    CHECK(static_cast<bool>(snapshot.memory));
    CHECK_EQ(snapshot.memory.value().total_physical_bytes, static_cast<std::uint64_t>(2000));
}
