#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "cli/config.hpp"
#include "monitor/engine.hpp"
#include "platform/windows/cpu_monitor.hpp"
#include "platform/windows/disk_monitor.hpp"
#include "platform/windows/memory_monitor.hpp"
#include "platform/windows/network_monitor.hpp"
#include "platform/windows/process_monitor.hpp"
#include "platform/windows/system_monitor.hpp"
#include "presentation/renderer.hpp"

namespace {

// How long the interval-based monitors (CPU, process, network) need
// between their baseline reading and their first real sample before that
// sample means anything. Independent of the user's --interval, which
// governs the refresh cadence *after* startup and can be far longer.
constexpr auto kBaselineWindow = std::chrono::milliseconds(200);

std::atomic<bool> g_shutdown_requested{false};

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    switch (ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            g_shutdown_requested.store(true);
            return TRUE; // handled: suppresses the default (immediate-kill) action
        default:
            return FALSE;
    }
}

// Sleeps for `total`, waking every 100ms to re-check the shutdown flag, so
// Ctrl+C is noticed promptly even when --interval is minutes or an hour.
void interruptible_sleep(std::chrono::milliseconds total) {
    constexpr auto slice = std::chrono::milliseconds(100);
    auto remaining = total;
    while (remaining > std::chrono::milliseconds::zero() && !g_shutdown_requested.load()) {
        const auto this_slice = std::min(slice, remaining);
        std::this_thread::sleep_for(this_slice);
        remaining -= this_slice;
    }
}

// Best-effort: lets the presentation layer's screen-clear escape codes work
// in older consoles that don't default to interpreting them. If this fails
// (very old conhost), the escape codes print as harmless garbage text
// instead of clearing the screen -- degraded, not broken.
void enable_ansi_escape_codes() {
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(out, &mode)) {
        return;
    }
    SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

// Repaint without the flash a full-screen erase causes: park the cursor at
// the top-left and overwrite the previous frame in place, then erase only
// whatever the last (longer) frame left below the new one. "\x1b[H" homes
// the cursor; "\x1b[0J" clears from the cursor to the end of the screen.
void cursor_home() { std::cout << "\x1b[H"; }
void clear_below_cursor() { std::cout << "\x1b[0J"; }

srm::monitor::MonitorEngine make_engine() {
    using namespace srm::platform::windows;
    return srm::monitor::MonitorEngine(std::make_unique<CpuMonitor>(), std::make_unique<MemoryMonitor>(),
                                        std::make_unique<DiskMonitor>(), std::make_unique<ProcessMonitor>(),
                                        std::make_unique<NetworkMonitor>(), std::make_unique<SystemMonitor>());
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);

    const auto parsed = srm::cli::parse_args(args);
    if (!parsed) {
        std::cerr << "Error: " << parsed.error().message << "\n\n" << srm::cli::usage_text();
        return 1;
    }

    const auto& config = parsed.value();
    if (config.help_requested) {
        std::cout << srm::cli::usage_text();
        return 0;
    }

    auto engine = make_engine();
    engine.poll(); // throwaway: primes the interval-based monitors' baselines
    std::this_thread::sleep_for(kBaselineWindow);

    if (config.once) {
        std::cout << srm::presentation::render(engine.poll(), config);
        return 0;
    }

    enable_ansi_escape_codes();
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    while (!g_shutdown_requested.load()) {
        cursor_home();
        std::cout << srm::presentation::render(engine.poll(), config);
        clear_below_cursor();
        std::cout.flush();
        interruptible_sleep(config.interval);
    }

    std::cout << "\nShutting down.\n";
    return 0;
}
