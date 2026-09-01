# Architecture

## Layers

```
main.cpp
  |
  |-- cli/          argv -> validated Config (no I/O)
  |-- presentation/  SystemSnapshot + Config -> report string (no I/O)
  |-- monitor/       MonitorEngine: bundles one sample() per monitor
  |-- platform/      interfaces.hpp (OS-agnostic contracts)
  |     `-- windows/ concrete Win32 implementations
  `-- core/          Result<T>, snapshot structs, pure math/format helpers
```

Dependencies only point downward: `core/` depends on nothing else in this
project; `platform/` depends on `core/`; `monitor/` depends on `core/` and
`platform/interfaces.hpp` (never on a concrete `platform/<os>/` backend);
`cli/` and `presentation/` depend on `core/` (and `presentation/` on
`monitor/` and `cli/` for the types it renders); `main.cpp` is the only
file that knows concrete Windows types exist at all -- it's where they're
constructed and injected into `MonitorEngine`.

## Data flow, one refresh cycle

```
main.cpp: engine.poll()
  -> MonitorEngine calls sample() on all six monitors
       ICpuMonitor::sample()     -> platform::windows::CpuMonitor
       IMemoryMonitor::sample()  -> platform::windows::MemoryMonitor
       IDiskMonitor::sample()    -> platform::windows::DiskMonitor
       IProcessMonitor::sample() -> platform::windows::ProcessMonitor
       INetworkMonitor::sample() -> platform::windows::NetworkMonitor
       ISystemMonitor::sample()  -> platform::windows::SystemMonitor
  -> six independent core::Result<T> values bundled into one SystemSnapshot
main.cpp: presentation::render(snapshot, config)
  -> one report string, each section degrading independently on failure
main.cpp: clear screen, print, interruptible_sleep(config.interval)
```

## Why `Result<T>` instead of exceptions or `std::expected`

Every monitor can fail independently at runtime for reasons that are
*expected*, not exceptional: a removable drive with no media, a process
that exited between enumeration and query, a counter this OS build doesn't
expose. `presentation::render` needs to keep showing the other five
sections when one fails, so "this metric is unavailable right now" has to
be a value the caller is forced to check, not a control-flow event a
caller might forget to catch. `std::expected<T, E>` would express the same
idea, but it's a C++23 library feature; this project targets C++20, and
`platform/interfaces.hpp` is meant to compile with any reasonably current
C++20 compiler rather than depending on which standard library shipped
`std::expected`. `core::Result<T>` (`src/core/result.hpp`) is the ~90-line
substitute.

## The `Raw*` + `translate()` pattern

Every Windows monitor (see `src/platform/windows/*_monitor.{hpp,cpp}`) is
split into two pieces:

- A `Raw*` plain-data struct plus a pure `translate()` function: no Win32
  types, just the actual success/failure and mapping logic. Unit tested
  directly with synthetic inputs, including failure cases (e.g. a simulated
  `GetLastError()`) that can't be reliably provoked by calling the real
  Windows API from a test.
- The `sample()` method: a thin wrapper that calls the real Win32 API,
  populates a `Raw*`, and hands it to `translate()`. Covered instead by an
  integration test that asserts invariants (`total >= available`,
  `percent` in `[0, 100]`, the test's own PID appears in the process list)
  rather than exact values, since there's no fixed "correct" answer for
  how much RAM or CPU load a given machine has.

This is why every `*_monitor_translate_tests.cpp` / `*_monitor_integration_tests.cpp`
pair exists side by side in `tests/platform/windows/`.

## Per-monitor design decisions

**CPU (`cpu_monitor`)** uses `GetSystemTimes`, not a PDH counter. On
Windows, `GetSystemTimes`' kernel time already includes idle time, so
`(idle, kernel+user)` is exactly the `(idle, total)` tick-pair model
`core::math::cpu_percent_from_ticks` expects, and it normalizes correctly
across core counts without needing to know how many cores exist. Using PDH
instead would mean the OS does the delta math internally, leaving
`cpu_percent_from_ticks` unexercised by real code -- unnecessary
complexity for no behavioral gain here.

Per-core utilization is **out of scope**: it requires
`NtQuerySystemInformation(SystemProcessorPerformanceInformation)`, an
undocumented (if long-stable) NTAPI call. Given the "no unnecessary
complexity" constraint, `CpuSnapshot::per_core_utilization_percent` is
always empty from the Windows backend.

**Process (`process_monitor`)** CPU% is **not clamped to 100**. A process
with several threads spread across multiple cores can legitimately consume
more than one core's worth of time per wall-clock second (200%, 400%, ...).
Clamping would hide that signal; this matches the classic
top/Task-Manager convention prior to per-core normalization. The math
lives in `core::math::cpu_percent_of_wall_time`, parameterized on
`ticks_per_second` specifically so a Linux backend (jiffies at
`sysconf(_SC_CLK_TCK)`, not FILETIME's fixed 100ns) can reuse it unchanged.

A process that can't be opened even at `PROCESS_QUERY_LIMITED_INFORMATION`
(protected/system processes such as `Secure System`, `Registry`) still
appears in the list with its pid and name -- Toolhelp32 doesn't need a
handle for that -- just with `cpu_percent` and `working_set_bytes` left at
0, the same value a genuinely idle, memory-light process would show. This
project does not distinguish "measured zero" from "couldn't measure" per
field. A `bool metrics_available` flag on `ProcessInfo` would resolve the
ambiguity but adds a field every consumer (presentation, tests) has to
handle for a distinction that rarely matters in practice; left as a known
limitation rather than solved speculatively.

**Disk (`disk_monitor`)** takes the opposite policy from Process: a volume
that exists but can't be queried right now (no media in an optical drive, a
share that just disconnected) is **omitted** from the result entirely,
rather than listed with zeroed fields. Unlike a process, a disk row with
`0 bytes free of 0 bytes total` reads as "this disk is full," which is
actively misleading rather than merely uninformative -- so the two
monitors intentionally disagree on how to represent "couldn't measure."

**Network (`network_monitor`)** filters out three categories of
`GetIfTable2` rows: loopback, operationally-down, and -- found only by
running the real output and reading it, not anticipated up front --
**filter-driver interfaces**
(`MIB_IF_ROW2.InterfaceAndOperStatusFlags.FilterInterface`). Windows'
NDIS lightweight-filter bindings (WFP, QoS Packet Scheduler) each show up
as their own row mirroring the exact same traffic as the physical adapter
underneath; without excluding them, one Wi-Fi adapter rendered as five or
six duplicate rows with identical throughput. `network_monitor.cpp` also
documents a real build gotcha: `MIB_IF_TABLE2` silently disappears from
`netioapi.h` unless `winsock2.h`/`ws2tcpip.h` are included *before*
`windows.h`/`iphlpapi.h`, because that struct is gated on `_WS2IPDEF_`
already being defined.

**System (`system_monitor`)** treats hostname lookup failure (rare,
theoretical) as a degrade-to-empty-string case rather than failing the
whole sample, since `GetTickCount64` for uptime has no realistic failure
mode that would justify making the caller handle an all-or-nothing Result
for two independent, cheap OS reads.

## Testing strategy

Three tiers, visible directly in `tests/`:

1. **Pure logic** (`core::math`, `core::format`, `cli::parse_args`,
   `presentation::render`, every `*_translate_tests.cpp`) -- no OS
   dependency, exercised with synthetic inputs including edge cases
   (counter resets, zero-capacity volumes, malformed arguments) that are
   hard or impossible to provoke from the real APIs.
2. **Dependency-injected orchestration** (`tests/monitor/engine_tests.cpp`
   + `tests/monitor/fake_monitors.hpp`) -- verifies `MonitorEngine` bundles
   results correctly and that one monitor failing doesn't affect the
   others, using fake monitors instead of real hardware.
3. **Real-API integration** (every `*_integration_tests.cpp`) -- calls the
   actual Windows API and asserts invariants (`total >= available`, own PID
   present, non-negative rates) rather than exact values, since "how much
   RAM does this machine have" has no fixed correct answer to assert
   against.

The test framework itself (`tests/support/test_framework.hpp`) is
hand-rolled rather than a vendored dependency (Catch2/doctest/GoogleTest):
the needed surface -- register a named test, assert a condition, report a
pass/fail summary -- is small enough that owning ~90 lines is simpler than
depending on it, and it keeps the "no unnecessary third-party libraries"
constraint literally true rather than just mostly true.

## Adding a Linux backend

Nothing above this line changes. `platform/interfaces.hpp` is already
OS-agnostic; a `platform/linux/` directory implementing the same six
interfaces is the only new code, wired into `src/platform/CMakeLists.txt`'s
existing `if (WIN32) ... else()` branch (which currently fails the
configure step on purpose rather than shipping a monitor-less binary).
This wasn't implemented in this project because the development
environment had no Linux/macOS toolchain available to build or test it
against -- writing it anyway would mean shipping code that has never
compiled, which the "identify and fix the root cause rather than masking
the symptom" process this project followed throughout argues against.
Concretely, each interface maps to:

| Interface | Windows source | Linux equivalent |
|---|---|---|
| `ICpuMonitor` | `GetSystemTimes` | `/proc/stat`'s `cpu` line: `idle+iowait` vs. the sum of all fields. Same `core::math::cpu_percent_from_ticks` call, unchanged. |
| `IMemoryMonitor` | `GlobalMemoryStatusEx` | `/proc/meminfo`: `MemTotal`, `MemAvailable`, `SwapTotal`, `SwapFree`. |
| `IDiskMonitor` | `GetLogicalDrives` + `GetDiskFreeSpaceExW` | Mount points from `/proc/mounts` (or `getmntent`), sizes from `statvfs()` per mount point. |
| `IProcessMonitor` | Toolhelp32 + `GetProcessTimes`/`GetProcessMemoryInfo` | Enumerate `/proc/[pid]/`, read `utime`+`stime` (fields 14/15) from `/proc/[pid]/stat` for CPU ticks, `VmRSS` from `/proc/[pid]/status` for memory. `core::math::cpu_percent_of_wall_time`'s `ticks_per_second` becomes `sysconf(_SC_CLK_TCK)` (typically 100) instead of FILETIME's fixed 10,000,000. |
| `INetworkMonitor` | `GetIfTable2` | `/proc/net/dev`'s per-interface `rx_bytes`/`tx_bytes` columns; filter `lo` instead of `IF_TYPE_SOFTWARE_LOOPBACK`, and there's no NDIS-filter-interface equivalent to exclude. Same `core::math::rate_per_second` call, unchanged. |
| `ISystemMonitor` | `GetTickCount64` + `GetComputerNameExW` | `/proc/uptime`'s first field; `gethostname()`. |

The two interval-based math functions in `core/math.hpp` need zero changes
for a Linux backend -- confirmation that the abstraction boundary was drawn
in the right place.

## Known limitations

- Per-core CPU utilization is not implemented (see above).
- Process CPU/memory fields don't distinguish "genuinely zero" from
  "couldn't be measured" (see above).
- ANSI screen-clear escape codes degrade to visible garbage text on a
  console old enough that `ENABLE_VIRTUAL_TERMINAL_PROCESSING` isn't
  supported, rather than falling back to a different clear mechanism.
- Ctrl+C shutdown latency is bounded by the 100ms sleep-slice size in
  `main.cpp`, not instantaneous.
- `ProcessMonitor::sample()` opens a handle to every running process on
  every poll; on a machine with an unusually large process count this is
  the most expensive of the six monitors per cycle, though still well
  within "the monitor itself shouldn't be a significant load" at normal
  process counts and refresh intervals.
