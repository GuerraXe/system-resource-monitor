#include "platform/windows/process_monitor.hpp"
#include "support/test_framework.hpp"

using srm::platform::windows::RawProcessSample;
using srm::platform::windows::translate;

TEST_CASE("process translate: known previous ticks yields a real cpu_percent") {
    RawProcessSample p;
    p.pid = 100;
    p.name = "app.exe";
    p.cpu_query_succeeded = true;
    p.cpu_ticks = 20'000'000; // 2 seconds of CPU time
    p.memory_query_succeeded = true;
    p.working_set_bytes = 4096;

    // 1 second ago this process had already consumed 10,000,000 ticks (1s);
    // it consumed another 1s of CPU time in 1 wall second -> 100%.
    const std::unordered_map<std::uint32_t, std::uint64_t> previous{{100, 10'000'000}};

    auto processes = translate({p}, previous, 1.0);

    CHECK_EQ(processes.size(), static_cast<std::size_t>(1));
    CHECK_EQ(processes[0].pid, static_cast<std::uint32_t>(100));
    CHECK_EQ(processes[0].name, std::string("app.exe"));
    CHECK(processes[0].cpu_percent == 100.0);
    CHECK_EQ(processes[0].working_set_bytes, static_cast<std::uint64_t>(4096));
}

TEST_CASE("process translate: a process absent from the previous sample reports 0% CPU") {
    RawProcessSample p;
    p.pid = 200;
    p.name = "new.exe";
    p.cpu_query_succeeded = true;
    p.cpu_ticks = 50'000'000;

    auto processes = translate({p}, {}, 1.0); // empty previous_ticks map

    CHECK_EQ(processes.size(), static_cast<std::size_t>(1));
    CHECK(processes[0].cpu_percent == 0.0);
}

TEST_CASE("process translate: a failed CPU query leaves cpu_percent at 0 without dropping the process") {
    RawProcessSample p;
    p.pid = 4;
    p.name = "System";
    p.cpu_query_succeeded = false; // e.g. access denied opening the handle

    auto processes = translate({p}, {{4, 999}}, 1.0);

    CHECK_EQ(processes.size(), static_cast<std::size_t>(1));
    CHECK_EQ(processes[0].pid, static_cast<std::uint32_t>(4));
    CHECK(processes[0].cpu_percent == 0.0);
}

TEST_CASE("process translate: a failed memory query leaves working_set_bytes at 0") {
    RawProcessSample p;
    p.pid = 8;
    p.name = "protected.exe";
    p.memory_query_succeeded = false;

    auto processes = translate({p}, {}, 1.0);

    CHECK_EQ(processes.size(), static_cast<std::size_t>(1));
    CHECK_EQ(processes[0].working_set_bytes, static_cast<std::uint64_t>(0));
}

TEST_CASE("process translate: empty input yields an empty result") {
    auto processes = translate({}, {}, 1.0);
    CHECK_EQ(processes.size(), static_cast<std::size_t>(0));
}
