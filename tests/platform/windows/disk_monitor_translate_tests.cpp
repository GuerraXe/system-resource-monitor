#include "platform/windows/disk_monitor.hpp"
#include "support/test_framework.hpp"

using srm::platform::windows::RawVolumeQuery;
using srm::platform::windows::translate;

TEST_CASE("disk translate: successful queries map through with fields intact") {
    RawVolumeQuery q;
    q.mount_point = "C:\\";
    q.space_query_succeeded = true;
    q.total_bytes = 500;
    q.free_bytes = 200;
    q.filesystem = "NTFS";

    auto volumes = translate({q});

    CHECK_EQ(volumes.size(), static_cast<std::size_t>(1));
    CHECK_EQ(volumes[0].mount_point, std::string("C:\\"));
    CHECK_EQ(volumes[0].total_bytes, static_cast<std::uint64_t>(500));
    CHECK_EQ(volumes[0].free_bytes, static_cast<std::uint64_t>(200));
    CHECK_EQ(volumes[0].filesystem, std::string("NTFS"));
}

TEST_CASE("disk translate: a failed space query is dropped, not reported as an empty volume") {
    RawVolumeQuery ok;
    ok.mount_point = "C:\\";
    ok.space_query_succeeded = true;
    ok.total_bytes = 500;
    ok.free_bytes = 200;

    RawVolumeQuery failed; // e.g. an optical drive with no media inserted
    failed.mount_point = "D:\\";
    failed.space_query_succeeded = false;

    auto volumes = translate({ok, failed});

    CHECK_EQ(volumes.size(), static_cast<std::size_t>(1));
    CHECK_EQ(volumes[0].mount_point, std::string("C:\\"));
}

TEST_CASE("disk translate: a successful space query with unknown filesystem keeps the volume") {
    RawVolumeQuery q;
    q.mount_point = "E:\\";
    q.space_query_succeeded = true;
    q.total_bytes = 100;
    q.free_bytes = 50;
    // filesystem left empty: GetVolumeInformationW failed independently of
    // the space query succeeding.

    auto volumes = translate({q});

    CHECK_EQ(volumes.size(), static_cast<std::size_t>(1));
    CHECK_EQ(volumes[0].filesystem, std::string(""));
}

TEST_CASE("disk translate: empty input yields an empty result") {
    auto volumes = translate({});
    CHECK_EQ(volumes.size(), static_cast<std::size_t>(0));
}
