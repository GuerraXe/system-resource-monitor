#include <iostream>

#include "core/format.hpp"

// Placeholder entry point. Argument parsing, the monitor engine, and the
// presentation layer are wired in during later milestones. The format call
// below is a light integration check that src/core links into srm cleanly.
int main() {
    std::cout << "System Resource Monitor - build scaffold OK ("
              << srm::core::format::bytes(1536) << ")\n";
    return 0;
}
