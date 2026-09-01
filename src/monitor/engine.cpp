#include "engine.hpp"

#include <utility>

namespace srm::monitor {

MonitorEngine::MonitorEngine(std::unique_ptr<platform::ICpuMonitor> cpu,
                              std::unique_ptr<platform::IMemoryMonitor> memory,
                              std::unique_ptr<platform::IDiskMonitor> disk,
                              std::unique_ptr<platform::IProcessMonitor> process,
                              std::unique_ptr<platform::INetworkMonitor> network,
                              std::unique_ptr<platform::ISystemMonitor> system)
    : cpu_(std::move(cpu)),
      memory_(std::move(memory)),
      disk_(std::move(disk)),
      process_(std::move(process)),
      network_(std::move(network)),
      system_(std::move(system)) {}

SystemSnapshot MonitorEngine::poll() {
    return SystemSnapshot{
        .cpu = cpu_->sample(),
        .memory = memory_->sample(),
        .disks = disk_->sample(),
        .processes = process_->sample(),
        .network = network_->sample(),
        .system = system_->sample(),
    };
}

} // namespace srm::monitor
