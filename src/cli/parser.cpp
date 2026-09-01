#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <sstream>

namespace srm::cli {

namespace {

std::optional<long long> parse_positive_integer(const std::string& text) {
    long long value = 0;
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end || value <= 0) {
        return std::nullopt;
    }
    return value;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

core::Result<Config> fail(const std::string& message) {
    return core::Result<Config>::Fail(core::Error{core::ErrorCode::InvalidArgument, message});
}

} // namespace

core::Result<Config> parse_args(const std::vector<std::string>& args) {
    Config config;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "--help" || arg == "-h") {
            config.help_requested = true;
            return core::Result<Config>::Ok(config);
        }

        if (arg == "--once") {
            config.once = true;
            continue;
        }

        if (arg == "--interval") {
            if (i + 1 >= args.size()) {
                return fail("--interval requires a value in milliseconds");
            }
            const std::string& value_text = args[++i];
            const auto parsed = parse_positive_integer(value_text);
            if (!parsed) {
                return fail("--interval value must be a positive integer (milliseconds): '" + value_text + "'");
            }
            const std::chrono::milliseconds interval{*parsed};
            if (interval < kMinInterval || interval > kMaxInterval) {
                return fail("--interval must be between " + std::to_string(kMinInterval.count()) + " and " +
                            std::to_string(kMaxInterval.count()) + " milliseconds, got " + value_text);
            }
            config.interval = interval;
            continue;
        }

        if (arg == "--top") {
            if (i + 1 >= args.size()) {
                return fail("--top requires a value (number of processes to display)");
            }
            const std::string& value_text = args[++i];
            const auto parsed = parse_positive_integer(value_text);
            if (!parsed || static_cast<std::size_t>(*parsed) > kMaxTopN) {
                return fail("--top value must be a positive integer up to " + std::to_string(kMaxTopN) +
                            ", got '" + value_text + "'");
            }
            config.top_n = static_cast<std::size_t>(*parsed);
            continue;
        }

        if (arg == "--sort") {
            if (i + 1 >= args.size()) {
                return fail("--sort requires a value (cpu or memory)");
            }
            const std::string value_text = to_lower(args[++i]);
            if (value_text == "cpu") {
                config.sort_key = SortKey::Cpu;
            } else if (value_text == "memory") {
                config.sort_key = SortKey::Memory;
            } else {
                return fail("--sort must be 'cpu' or 'memory', got '" + args[i] + "'");
            }
            continue;
        }

        return fail("unknown argument: '" + arg + "'");
    }

    return core::Result<Config>::Ok(config);
}

std::string usage_text() {
    std::ostringstream oss;
    oss << "System Resource Monitor\n"
        << "\n"
        << "Usage: srm [options]\n"
        << "\n"
        << "Options:\n"
        << "  --interval <ms>   Refresh interval in milliseconds (default: 1000)\n"
        << "  --once            Print a single snapshot and exit\n"
        << "  --top <n>         Number of processes to display, sorted by --sort (default: 5)\n"
        << "  --sort <key>      Process sort key: cpu or memory (default: cpu)\n"
        << "  -h, --help        Show this help message\n";
    return oss.str();
}

} // namespace srm::cli
