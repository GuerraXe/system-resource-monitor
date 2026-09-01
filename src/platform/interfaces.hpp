#pragma once

// OS-agnostic contracts every backend implements. This header must never
// include a platform SDK header (<windows.h>, <sys/...>, ...) -- code
// outside platform/<os>/ depends only on this file and core/, so it stays
// buildable and meaningful regardless of which concrete backend is linked
// in. Grown incrementally: one interface is added per milestone, once its
// concrete implementation has actually been written and tested, rather than
// speculatively designed up front.

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

} // namespace srm::platform
