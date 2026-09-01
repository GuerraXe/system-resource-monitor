#pragma once

// OS-agnostic contracts every backend implements. This header must never
// include a platform SDK header (<windows.h>, <sys/...>, ...) -- code
// outside platform/<os>/ depends only on this file and core/, so it stays
// buildable and meaningful regardless of which concrete backend is linked
// in. Grown incrementally: one interface is added per milestone, once its
// concrete implementation has actually been written and tested, rather than
// speculatively designed up front.

#include <vector>

#include "core/result.hpp"
#include "core/snapshot.hpp"

namespace srm::platform {

// A fresh MemoryInfo sample is always instantaneous (no interval state),
// so unlike CPU/process/network monitors this interface needs no
// "first sample is a throwaway" caveat.
class IMemoryMonitor {
public:
    virtual ~IMemoryMonitor() = default;
    virtual core::Result<core::MemoryInfo> sample() = 0;
};

// The outer Result models enumeration itself failing (no OS-level way to
// even list volumes). A volume that exists but can't be queried right now
// (no media in a drive, a share that just disconnected, ...) is simply
// omitted from the vector rather than failing the whole sample -- that
// distinction is "not currently usable media," not an error worth
// surfacing per volume.
class IDiskMonitor {
public:
    virtual ~IDiskMonitor() = default;
    virtual core::Result<std::vector<core::DiskVolumeInfo>> sample() = 0;
};

// CPU utilization is interval-based: it isn't meaningful from a single
// cumulative tick reading. Each backend takes its first reading at
// construction time, so sample() is meaningful starting with its very
// first call -- callers never need a separate throwaway warm-up call --
// though that first percentage necessarily reflects however little time
// has elapsed since construction rather than a full refresh interval.
class ICpuMonitor {
public:
    virtual ~ICpuMonitor() = default;
    virtual core::Result<core::CpuSnapshot> sample() = 0;
};

// Interval-based like ICpuMonitor: a process's cpu_percent needs a previous
// reading to diff against. Unlike ICpuMonitor, no separate baseline call is
// needed -- a process absent from the previous sample (new since then, or
// this is the very first call) simply reports 0% CPU, since "no data yet"
// and "genuinely idle" aren't distinguishable from a single reading anyway.
class IProcessMonitor {
public:
    virtual ~IProcessMonitor() = default;
    virtual core::Result<std::vector<core::ProcessInfo>> sample() = 0;
};

// Interval-based like IProcessMonitor: throughput needs a previous
// cumulative-byte-counter reading to diff against, and an interface with no
// prior reading (new since last sample, or this is the first call) simply
// reports 0 bytes/sec rather than needing a warm-up call or an error path.
class INetworkMonitor {
public:
    virtual ~INetworkMonitor() = default;
    virtual core::Result<std::vector<core::NetworkInterfaceInfo>> sample() = 0;
};

} // namespace srm::platform
