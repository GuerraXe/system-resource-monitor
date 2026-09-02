#include "renderer.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "core/format.hpp"
#include "core/math.hpp"

namespace srm::presentation {

namespace {

void render_system(std::ostringstream& out, const monitor::SystemSnapshot& snapshot) {
    if (snapshot.system) {
        const auto& info = snapshot.system.value();
        out << info.hostname << "  uptime "
            << core::format::duration_seconds(static_cast<std::uint64_t>(info.uptime.count())) << "\n";
    } else {
        out << "System: unavailable (" << snapshot.system.error().message << ")\n";
    }
}

void render_cpu(std::ostringstream& out, const monitor::SystemSnapshot& snapshot) {
    out << "CPU: ";
    if (snapshot.cpu) {
        out << core::format::percent(snapshot.cpu.value().total_utilization_percent) << "\n";
    } else {
        out << "unavailable (" << snapshot.cpu.error().message << ")\n";
    }
}

void render_memory(std::ostringstream& out, const monitor::SystemSnapshot& snapshot) {
    out << "Memory: ";
    if (snapshot.memory) {
        const auto& mem = snapshot.memory.value();
        const std::uint64_t used = mem.total_physical_bytes >= mem.available_physical_bytes
                                        ? mem.total_physical_bytes - mem.available_physical_bytes
                                        : 0;
        const double used_percent = core::math::percent_used(mem.total_physical_bytes, mem.available_physical_bytes);
        out << core::format::bytes(used) << " / " << core::format::bytes(mem.total_physical_bytes) << " ("
            << core::format::percent(used_percent) << " used)\n";
    } else {
        out << "unavailable (" << snapshot.memory.error().message << ")\n";
    }
}

void render_disks(std::ostringstream& out, const monitor::SystemSnapshot& snapshot) {
    out << "Disks:\n";
    if (!snapshot.disks) {
        out << "  unavailable (" << snapshot.disks.error().message << ")\n";
        return;
    }
    if (snapshot.disks.value().empty()) {
        out << "  (none detected)\n";
        return;
    }
    for (const auto& v : snapshot.disks.value()) {
        const std::uint64_t used = v.total_bytes >= v.free_bytes ? v.total_bytes - v.free_bytes : 0;
        const double used_percent = core::math::percent_used(v.total_bytes, v.free_bytes);
        out << "  " << v.mount_point << " (" << (v.filesystem.empty() ? "unknown" : v.filesystem) << ")  "
            << core::format::bytes(used) << " / " << core::format::bytes(v.total_bytes) << " ("
            << core::format::percent(used_percent) << " used)\n";
    }
}

void render_network(std::ostringstream& out, const monitor::SystemSnapshot& snapshot) {
    out << "Network:\n";
    if (!snapshot.network) {
        out << "  unavailable (" << snapshot.network.error().message << ")\n";
        return;
    }
    if (snapshot.network.value().empty()) {
        out << "  (no active interfaces)\n";
        return;
    }
    for (const auto& iface : snapshot.network.value()) {
        out << "  " << iface.name << "  down "
            << core::format::bytes(static_cast<std::uint64_t>(iface.receive_bytes_per_second)) << "/s  up "
            << core::format::bytes(static_cast<std::uint64_t>(iface.send_bytes_per_second)) << "/s\n";
    }
}

void render_processes(std::ostringstream& out, const monitor::SystemSnapshot& snapshot, const cli::Config& config) {
    if (!snapshot.processes) {
        out << "Processes: unavailable (" << snapshot.processes.error().message << ")\n";
        return;
    }

    auto processes = snapshot.processes.value();
    std::sort(processes.begin(), processes.end(), [&](const auto& a, const auto& b) {
        if (config.sort_key == cli::SortKey::Cpu) {
            return a.cpu_percent > b.cpu_percent;
        }
        return a.working_set_bytes > b.working_set_bytes;
    });

    const char* sort_label = config.sort_key == cli::SortKey::Cpu ? "CPU" : "memory";
    out << "Processes (" << processes.size() << " total, top " << config.top_n << " by " << sort_label << "):\n";

    const std::size_t shown = std::min(config.top_n, processes.size());

    // Pre-format each column, then pad to a common width so the rows line
    // up regardless of how wide any individual pid / name / percentage is.
    std::vector<std::string> pid_cells, name_cells, cpu_cells, mem_cells;
    pid_cells.reserve(shown);
    name_cells.reserve(shown);
    cpu_cells.reserve(shown);
    mem_cells.reserve(shown);
    for (std::size_t i = 0; i < shown; ++i) {
        const auto& p = processes[i];
        pid_cells.push_back(std::to_string(p.pid));
        name_cells.push_back(p.name);
        cpu_cells.push_back(core::format::percent(p.cpu_percent));
        mem_cells.push_back(core::format::bytes(p.working_set_bytes));
    }

    const auto max_width = [](const std::vector<std::string>& cells) {
        std::size_t width = 0;
        for (const auto& cell : cells) {
            width = std::max(width, cell.size());
        }
        return width;
    };
    const std::size_t pid_width = max_width(pid_cells);
    const std::size_t name_width = max_width(name_cells);
    const std::size_t cpu_width = max_width(cpu_cells);

    const auto pad_left = [](const std::string& s, std::size_t width) {
        return s.size() >= width ? s : std::string(width - s.size(), ' ') + s;
    };
    const auto pad_right = [](const std::string& s, std::size_t width) {
        return s.size() >= width ? s : s + std::string(width - s.size(), ' ');
    };

    for (std::size_t i = 0; i < shown; ++i) {
        out << "  " << pad_left(pid_cells[i], pid_width) << "  " << pad_right(name_cells[i], name_width) << "  "
            << pad_left(cpu_cells[i], cpu_width) << "  " << mem_cells[i] << "\n";
    }
}

} // namespace

std::string render(const monitor::SystemSnapshot& snapshot, const cli::Config& config) {
    std::ostringstream out;
    render_system(out, snapshot);
    render_cpu(out, snapshot);
    render_memory(out, snapshot);
    render_disks(out, snapshot);
    render_network(out, snapshot);
    render_processes(out, snapshot, config);
    return out.str();
}

} // namespace srm::presentation
