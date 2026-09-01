# System Resource Monitor

A lightweight, dependency-free CLI that shows what a Windows machine is
doing right now: CPU load, memory and disk usage, per-process CPU/memory,
per-interface network throughput, and uptime -- refreshed live in the
terminal or as a single snapshot.

This isn't an attempt to replace Task Manager. It exists to demonstrate
real systems programming: talking directly to OS APIs, modeling failure as
data instead of exceptions, separating pure logic from I/O so it can
actually be unit tested, and keeping the platform-specific 20% of the code
cleanly isolated from the 80% that doesn't need to know which OS it's
running on.

## Why C++

Every metric here comes from an OS API that returns raw counters, kernel
structs, or process handles -- `GetSystemTimes`, `GetProcessMemoryInfo`,
`CreateToolhelp32Snapshot`, the IP Helper API. That's the domain C and C++
were built for, and C++20 specifically buys real ergonomics over raw C
for this: RAII for handles, `std::chrono` for the interval math that
underpins every "utilization" or "throughput" number here, strong typing
for six independent metrics that must never be confused with each other,
and templates for one `Result<T>` type instead of a hand-rolled error path
per monitor.

## How it works

```
main.cpp
  |-- cli/          argv -> validated Config
  |-- monitor/       MonitorEngine samples all six monitors into one SystemSnapshot
  |-- platform/      OS-agnostic interfaces, implemented in platform/windows/
  |-- presentation/  SystemSnapshot -> terminal report
  `-- core/          Result<T>, shared data types, pure math/formatting
```

Every monitor fails independently: if one counter is unavailable, the
other five keep reporting rather than the whole tool crashing or going
silent. See [ARCHITECTURE.md](ARCHITECTURE.md) for the full design --
per-monitor decisions, the test strategy, and exactly what a Linux backend
would need to add to the existing interfaces.

## Building

Requires a C++20 toolchain. This project was built and tested against
**Visual Studio 2022 Build Tools (MSVC v14.44 / 17.14)**, using the CMake
and Ninja bundled with it -- no separate CMake or compiler install is
required if you already have Visual Studio 2022 (Build Tools or the full
IDE) with the "Desktop development with C++" workload.

From a **Developer PowerShell for VS 2022** (or after running
`vcvars64.bat`), from the project root:

```powershell
cmake --preset windows-msvc
cmake --build build/windows-msvc --config Debug
```

The `windows-msvc` preset uses the Visual Studio generator, which resolves
the MSVC toolset itself -- you don't need a Developer shell just to
*configure*, only if you invoke `cl.exe` directly for anything else.

Opening the folder in VS Code with the CMake Tools extension also works:
it detects `CMakePresets.json` and the installed Visual Studio kit
automatically.

## Running

```powershell
.\build\windows-msvc\src\Debug\srm.exe                          # continuous, 1s refresh
.\build\windows-msvc\src\Debug\srm.exe --once                    # single snapshot
.\build\windows-msvc\src\Debug\srm.exe --interval 500 --top 10   # refresh every 500ms, show 10 processes
.\build\windows-msvc\src\Debug\srm.exe --sort memory             # sort the process list by memory instead of CPU
.\build\windows-msvc\src\Debug\srm.exe --help                    # usage
```

Continuous mode redraws in place and exits cleanly on Ctrl+C.

### Example output (`--once --top 3`)

```
DESKTOP-EXAMPLE  uptime 1h 37m 57s
CPU: 7.1%
Memory: 14.9 GB / 31.7 GB (47.1% used)
Disks:
  C:\ (NTFS)  385.7 GB / 951.6 GB (40.5% used)
Network:
  Wi-Fi  down 13.8 KB/s  up 19.0 KB/s
Processes (320 total, top 3 by CPU):
  3652   chrome.exe       14.0%   597.9 MB
  13772  chrome.exe        7.0%   588.2 MB
  27568  model_host.exe    0.0%   415.1 MB
```

### Command-line options

| Flag | Default | Description |
|---|---|---|
| `--interval <ms>` | `1000` | Refresh interval in milliseconds (clamped to 100–3,600,000) |
| `--once` | off | Print a single snapshot and exit |
| `--top <n>` | `5` | Number of processes to display (clamped to 1–500) |
| `--sort <cpu\|memory>` | `cpu` | Process list sort key |
| `-h`, `--help` | | Show usage |

## Testing

```powershell
cmake --build build/windows-msvc --config Debug
ctest --test-dir build/windows-msvc -C Debug --output-on-failure
```

The suite mixes pure-logic unit tests (no OS dependency), dependency-injected
orchestration tests (fake monitors, no real hardware), and integration
tests that call the real Windows APIs and assert invariants rather than
exact values. See [ARCHITECTURE.md](ARCHITECTURE.md#testing-strategy) for
why it's split that way.

## Engineering challenges worth calling out

- **Modeling six independently-failing metrics** without exceptions or a
  third-party `Result`/`Expected` type -- `core::Result<T>` and the
  `Raw*` + `translate()` split used by every monitor (see
  [ARCHITECTURE.md](ARCHITECTURE.md)) exist specifically so failure paths
  are unit testable, not just theoretically handled.
- **Getting CPU/process/network percentages right at all**: these are
  interval-based, not instantaneous -- a single reading of a cumulative
  counter means nothing without a previous one to diff against. Getting
  the tick-unit math right (and reusable across CPU, per-process, and
  per-interface variants) is `core/math.hpp`'s entire job.
- **Windows API quirks that only show up by running the code**, not by
  reading the docs: `MIB_IF_TABLE2` silently vanishing from `netioapi.h`
  without a specific, non-obvious include order; `GetIfTable2` reporting
  five duplicate rows per physical adapter because of NDIS filter-driver
  bindings; MSVC requiring `/Zc:__cplusplus` just to make `__cplusplus`
  report the truth. Each is documented at the file where it was found.
