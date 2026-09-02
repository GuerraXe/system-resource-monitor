#include "platform/windows/memory_monitor.hpp"
#include "support/test_framework.hpp"

using srm::core::ErrorCode;
using srm::platform::windows::RawMemoryStatus;
using srm::platform::windows::translate;

// These test translate() directly with synthetic RawMemoryStatus values --
// no real GlobalMemoryStatusEx call, no <windows.h> dependency in this file.
// The failure case in particular can't be reliably provoked by calling the
// actual Windows API from a test.

TEST_CASE("memory translate: success maps every field through unchanged") {
    RawMemoryStatus raw;
    raw.succeeded = true;
    raw.total_physical_bytes = 16ull * 1024 * 1024 * 1024;
    raw.available_physical_bytes = 4ull * 1024 * 1024 * 1024;

    auto result = translate(raw);

    CHECK(static_cast<bool>(result));
    const auto& mem = result.value();
    CHECK_EQ(mem.total_physical_bytes, raw.total_physical_bytes);
    CHECK_EQ(mem.available_physical_bytes, raw.available_physical_bytes);
}

TEST_CASE("memory translate: failure produces a PlatformApiFailure with the error code in the message") {
    RawMemoryStatus raw;
    raw.succeeded = false;
    raw.last_error = 5; // ERROR_ACCESS_DENIED

    auto result = translate(raw);

    CHECK(!static_cast<bool>(result));
    CHECK(result.error().code == ErrorCode::PlatformApiFailure);
    CHECK(result.error().message.find("5") != std::string::npos);
}
