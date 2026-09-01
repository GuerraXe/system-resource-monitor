#include "presentation/renderer.hpp"
#include "support/test_framework.hpp"

namespace core = srm::core;
using srm::cli::Config;
using srm::cli::SortKey;
using srm::monitor::SystemSnapshot;
using srm::presentation::render;

namespace {

SystemSnapshot make_all_success_snapshot() {
    core::CpuSnapshot cpu;
    cpu.total_utilization_percent = 12.5;

    core::MemoryInfo mem;
    mem.total_physical_bytes = 1000;
    mem.available_physical_bytes = 400;

    core::DiskVolumeInfo disk;
    disk.mount_point = "C:\\";
    disk.filesystem = "NTFS";
    disk.total_bytes = 500;
    disk.free_bytes = 100;

    core::NetworkInterfaceInfo nic;
    nic.name = "Ethernet";
    nic.receive_bytes_per_second = 1024;
    nic.send_bytes_per_second = 512;

    core::ProcessInfo low_cpu_high_mem;
    low_cpu_high_mem.pid = 1;
    low_cpu_high_mem.name = "low_cpu_high_mem.exe";
    low_cpu_high_mem.cpu_percent = 1.0;
    low_cpu_high_mem.working_set_bytes = 9000;

    core::ProcessInfo high_cpu_low_mem;
    high_cpu_low_mem.pid = 2;
    high_cpu_low_mem.name = "high_cpu_low_mem.exe";
    high_cpu_low_mem.cpu_percent = 50.0;
    high_cpu_low_mem.working_set_bytes = 100;

    core::SystemInfo sys;
    sys.hostname = "test-host";
    sys.uptime = std::chrono::seconds(65);

    return SystemSnapshot{
        .cpu = core::Result<core::CpuSnapshot>::Ok(cpu),
        .memory = core::Result<core::MemoryInfo>::Ok(mem),
        .disks = core::Result<std::vector<core::DiskVolumeInfo>>::Ok({disk}),
        .processes = core::Result<std::vector<core::ProcessInfo>>::Ok({low_cpu_high_mem, high_cpu_low_mem}),
        .network = core::Result<std::vector<core::NetworkInterfaceInfo>>::Ok({nic}),
        .system = core::Result<core::SystemInfo>::Ok(sys),
    };
}

Config make_config(std::size_t top_n, SortKey sort_key) {
    Config config;
    config.top_n = top_n;
    config.sort_key = sort_key;
    return config;
}

} // namespace

TEST_CASE("render: an all-success snapshot includes every section's data") {
    auto text = render(make_all_success_snapshot(), make_config(5, SortKey::Cpu));

    CHECK(text.find("test-host") != std::string::npos);
    CHECK(text.find("1m 5s") != std::string::npos);
    CHECK(text.find("12.5%") != std::string::npos);
    CHECK(text.find("C:\\") != std::string::npos);
    CHECK(text.find("Ethernet") != std::string::npos);
    CHECK(text.find("high_cpu_low_mem.exe") != std::string::npos);
    CHECK(text.find("low_cpu_high_mem.exe") != std::string::npos);
}

TEST_CASE("render: sorts processes by CPU descending when sort_key is Cpu") {
    auto text = render(make_all_success_snapshot(), make_config(5, SortKey::Cpu));

    const auto high_pos = text.find("high_cpu_low_mem.exe");
    const auto low_pos = text.find("low_cpu_high_mem.exe");
    CHECK(high_pos != std::string::npos);
    CHECK(low_pos != std::string::npos);
    CHECK(high_pos < low_pos);
}

TEST_CASE("render: sorts processes by memory descending when sort_key is Memory") {
    auto text = render(make_all_success_snapshot(), make_config(5, SortKey::Memory));

    const auto high_pos = text.find("high_cpu_low_mem.exe");
    const auto low_pos = text.find("low_cpu_high_mem.exe");
    CHECK(high_pos != std::string::npos);
    CHECK(low_pos != std::string::npos);
    CHECK(low_pos < high_pos); // low_cpu_high_mem has the larger working set
}

TEST_CASE("render: top_n limits how many process rows appear, but not the reported total") {
    auto text = render(make_all_success_snapshot(), make_config(1, SortKey::Cpu));

    CHECK(text.find("2 total") != std::string::npos);
    CHECK(text.find("high_cpu_low_mem.exe") != std::string::npos);
    CHECK(text.find("low_cpu_high_mem.exe") == std::string::npos); // cut off by top_n = 1
}

TEST_CASE("render: a failed field shows its error message without affecting other sections") {
    auto snapshot = make_all_success_snapshot();
    snapshot.cpu = core::Result<core::CpuSnapshot>::Fail(core::Error{core::ErrorCode::Unavailable, "no counter"});

    auto text = render(snapshot, make_config(5, SortKey::Cpu));

    CHECK(text.find("unavailable (no counter)") != std::string::npos);
    // Memory section is untouched by the CPU failure.
    CHECK(text.find("test-host") != std::string::npos);
}

TEST_CASE("render: an empty disk list shows a none-detected message rather than nothing") {
    auto snapshot = make_all_success_snapshot();
    snapshot.disks = core::Result<std::vector<core::DiskVolumeInfo>>::Ok({});

    auto text = render(snapshot, make_config(5, SortKey::Cpu));

    CHECK(text.find("none detected") != std::string::npos);
}

TEST_CASE("render: an empty network list shows a no-active-interfaces message") {
    auto snapshot = make_all_success_snapshot();
    snapshot.network = core::Result<std::vector<core::NetworkInterfaceInfo>>::Ok({});

    auto text = render(snapshot, make_config(5, SortKey::Cpu));

    CHECK(text.find("no active interfaces") != std::string::npos);
}
