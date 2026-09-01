#pragma once

// Formats a monitor::SystemSnapshot into a human-readable terminal report.
// Pure (no I/O, no terminal control codes) so it's unit testable by
// checking the returned string -- the actual screen-clearing/redraw loop
// and terminal setup live in main.cpp, which just prints what this returns.

#include <string>

#include "cli/config.hpp"
#include "monitor/engine.hpp"

namespace srm::presentation {

std::string render(const monitor::SystemSnapshot& snapshot, const cli::Config& config);

} // namespace srm::presentation
