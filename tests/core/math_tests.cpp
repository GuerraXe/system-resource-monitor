#include "core/math.hpp"
#include "support/test_framework.hpp"

using namespace srm::core::math;

TEST_CASE("percent_used: half capacity available is 50%") {
    CHECK(percent_used(100, 50) == 50.0);
}

TEST_CASE("percent_used: zero total is defined as 0% used") {
    CHECK(percent_used(0, 0) == 0.0);
}

TEST_CASE("percent_used: fully available is 0% used") {
    CHECK(percent_used(100, 100) == 0.0);
}

TEST_CASE("percent_used: nothing available is 100% used") {
    CHECK(percent_used(100, 0) == 100.0);
}

TEST_CASE("percent_used: available exceeding total clamps to 0% used") {
    // Can happen with slightly inconsistent OS-reported figures.
    CHECK(percent_used(100, 150) == 0.0);
}

TEST_CASE("rate_per_second: simple positive delta over one second") {
    CHECK(rate_per_second(1000, 3000, 1.0) == 2000.0);
}

TEST_CASE("rate_per_second: delta spread over two seconds halves the rate") {
    CHECK(rate_per_second(1000, 3000, 2.0) == 1000.0);
}

TEST_CASE("rate_per_second: zero elapsed time yields zero, not a divide-by-zero") {
    CHECK(rate_per_second(1000, 3000, 0.0) == 0.0);
}

TEST_CASE("rate_per_second: counter reset (current < previous) yields zero") {
    CHECK(rate_per_second(5000, 100, 1.0) == 0.0);
}

TEST_CASE("cpu_percent_from_ticks: half idle over the interval is 50% busy") {
    // 1000 total ticks elapsed, 500 of them idle.
    CHECK(cpu_percent_from_ticks(0, 0, 500, 1000) == 50.0);
}

TEST_CASE("cpu_percent_from_ticks: fully idle interval is 0% busy") {
    CHECK(cpu_percent_from_ticks(0, 0, 1000, 1000) == 0.0);
}

TEST_CASE("cpu_percent_from_ticks: fully busy interval is 100% busy") {
    CHECK(cpu_percent_from_ticks(0, 0, 0, 1000) == 100.0);
}

TEST_CASE("cpu_percent_from_ticks: non-advancing total tick counter yields zero") {
    CHECK(cpu_percent_from_ticks(100, 1000, 150, 1000) == 0.0);
}

TEST_CASE("cpu_percent_from_ticks: idle delta exceeding total delta clamps to 0% busy") {
    // Guards against a slightly inconsistent counter pair rather than
    // producing a negative "busy" figure.
    CHECK(cpu_percent_from_ticks(0, 1000, 2000, 1100) == 0.0);
}

TEST_CASE("cpu_percent_of_wall_time: one full core-second of CPU time in one wall second is 100%") {
    // 10,000,000 ticks == 1 second at Windows FILETIME's 100ns resolution.
    CHECK(cpu_percent_of_wall_time(0, 10'000'000, 1.0, 10'000'000.0) == 100.0);
}

TEST_CASE("cpu_percent_of_wall_time: multi-threaded process can exceed 100% (not clamped)") {
    // Two full core-seconds of CPU time consumed in one wall second implies
    // the process used two cores simultaneously -- 200%, not clamped away.
    CHECK(cpu_percent_of_wall_time(0, 20'000'000, 1.0, 10'000'000.0) == 200.0);
}

TEST_CASE("cpu_percent_of_wall_time: no CPU time consumed is 0%") {
    CHECK(cpu_percent_of_wall_time(1000, 1000, 1.0, 10'000'000.0) == 0.0);
}

TEST_CASE("cpu_percent_of_wall_time: zero elapsed wall time yields zero, not a divide-by-zero") {
    CHECK(cpu_percent_of_wall_time(0, 10'000'000, 0.0, 10'000'000.0) == 0.0);
}

TEST_CASE("cpu_percent_of_wall_time: counter reset (current < previous) yields zero") {
    CHECK(cpu_percent_of_wall_time(10'000'000, 0, 1.0, 10'000'000.0) == 0.0);
}
