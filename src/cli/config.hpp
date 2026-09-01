#pragma once

// Command-line argument parsing and the resulting run configuration. Pure
// and platform-independent: takes a vector<string> rather than raw
// argv/argc so it's directly unit testable without constructing a fake
// argv array.

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include "core/result.hpp"

namespace srm::cli {

enum class SortKey { Cpu, Memory };

struct Config {
    std::chrono::milliseconds interval{1000};
    bool once = false;
    std::size_t top_n = 5;
    SortKey sort_key = SortKey::Cpu;
    bool help_requested = false;
};

// Refresh interval bounds: below kMinInterval the process-sampling
// overhead itself becomes a meaningful fraction of what's being measured
// (defeats "the monitor shouldn't itself be a significant load"); above
// kMaxInterval it's almost certainly a typo (an extra zero) rather than an
// intentional hour-plus refresh period.
inline constexpr std::chrono::milliseconds kMinInterval{100};
inline constexpr std::chrono::milliseconds kMaxInterval{3'600'000}; // 1 hour
inline constexpr std::size_t kMaxTopN = 500;

// Parses arguments (excluding argv[0]). Fails with a specific,
// human-readable message -- naming the offending flag and value -- on an
// unknown flag, a missing or non-numeric value, or a value outside the
// accepted range. --help/-h short-circuits the rest of parsing: once seen,
// Config::help_requested is set and parsing stops immediately, even if
// later arguments would otherwise be invalid.
core::Result<Config> parse_args(const std::vector<std::string>& args);

std::string usage_text();

} // namespace srm::cli
