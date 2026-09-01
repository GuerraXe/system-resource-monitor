#include "cli/config.hpp"
#include "support/test_framework.hpp"

using srm::cli::Config;
using srm::cli::parse_args;
using srm::cli::SortKey;
using srm::core::ErrorCode;

TEST_CASE("parse_args: no arguments yields all defaults") {
    auto result = parse_args({});

    CHECK(static_cast<bool>(result));
    const auto& config = result.value();
    CHECK(config.interval == std::chrono::milliseconds(1000));
    CHECK(!config.once);
    CHECK_EQ(config.top_n, static_cast<std::size_t>(5));
    CHECK(config.sort_key == SortKey::Cpu);
    CHECK(!config.help_requested);
}

TEST_CASE("parse_args: --once sets one-shot mode") {
    auto result = parse_args({"--once"});
    CHECK(static_cast<bool>(result));
    CHECK(result.value().once);
}

TEST_CASE("parse_args: --interval accepts a valid value") {
    auto result = parse_args({"--interval", "500"});
    CHECK(static_cast<bool>(result));
    CHECK(result.value().interval == std::chrono::milliseconds(500));
}

TEST_CASE("parse_args: --interval below the minimum is rejected") {
    auto result = parse_args({"--interval", "10"});
    CHECK(!static_cast<bool>(result));
    CHECK(result.error().code == ErrorCode::InvalidArgument);
}

TEST_CASE("parse_args: --interval above the maximum is rejected") {
    auto result = parse_args({"--interval", "99999999"});
    CHECK(!static_cast<bool>(result));
}

TEST_CASE("parse_args: --interval with a non-numeric value is rejected") {
    auto result = parse_args({"--interval", "soon"});
    CHECK(!static_cast<bool>(result));
    CHECK(result.error().code == ErrorCode::InvalidArgument);
}

TEST_CASE("parse_args: --interval with no value is rejected") {
    auto result = parse_args({"--interval"});
    CHECK(!static_cast<bool>(result));
}

TEST_CASE("parse_args: --top accepts a valid value") {
    auto result = parse_args({"--top", "10"});
    CHECK(static_cast<bool>(result));
    CHECK_EQ(result.value().top_n, static_cast<std::size_t>(10));
}

TEST_CASE("parse_args: --top of zero is rejected (not a positive integer)") {
    auto result = parse_args({"--top", "0"});
    CHECK(!static_cast<bool>(result));
}

TEST_CASE("parse_args: --top above the maximum is rejected") {
    auto result = parse_args({"--top", "10000"});
    CHECK(!static_cast<bool>(result));
}

TEST_CASE("parse_args: --sort accepts cpu and memory") {
    auto cpu_result = parse_args({"--sort", "cpu"});
    CHECK(static_cast<bool>(cpu_result));
    CHECK(cpu_result.value().sort_key == SortKey::Cpu);

    auto mem_result = parse_args({"--sort", "MEMORY"}); // case-insensitive
    CHECK(static_cast<bool>(mem_result));
    CHECK(mem_result.value().sort_key == SortKey::Memory);
}

TEST_CASE("parse_args: --sort with an invalid key is rejected") {
    auto result = parse_args({"--sort", "disk"});
    CHECK(!static_cast<bool>(result));
}

TEST_CASE("parse_args: an unknown flag is rejected with a specific message") {
    auto result = parse_args({"--bogus"});
    CHECK(!static_cast<bool>(result));
    CHECK(result.error().message.find("--bogus") != std::string::npos);
}

TEST_CASE("parse_args: --help short-circuits parsing even with invalid args after it") {
    auto result = parse_args({"--help", "--bogus"});
    CHECK(static_cast<bool>(result));
    CHECK(result.value().help_requested);
}

TEST_CASE("parse_args: combining multiple valid flags works together") {
    auto result = parse_args({"--interval", "2000", "--top", "3", "--sort", "memory", "--once"});
    CHECK(static_cast<bool>(result));
    const auto& config = result.value();
    CHECK(config.interval == std::chrono::milliseconds(2000));
    CHECK_EQ(config.top_n, static_cast<std::size_t>(3));
    CHECK(config.sort_key == SortKey::Memory);
    CHECK(config.once);
}

TEST_CASE("usage_text: mentions every documented flag") {
    auto text = srm::cli::usage_text();
    CHECK(text.find("--interval") != std::string::npos);
    CHECK(text.find("--once") != std::string::npos);
    CHECK(text.find("--top") != std::string::npos);
    CHECK(text.find("--sort") != std::string::npos);
    CHECK(text.find("--help") != std::string::npos);
}
