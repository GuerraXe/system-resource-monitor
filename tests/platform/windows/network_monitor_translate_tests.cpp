#include "platform/windows/network_monitor.hpp"
#include "support/test_framework.hpp"

using srm::platform::windows::RawInterfaceSample;
using srm::platform::windows::translate;

namespace {
using PreviousBytes = std::unordered_map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>>;
}

TEST_CASE("network translate: an up, non-loopback interface with a prior reading gets real rates") {
    RawInterfaceSample eth;
    eth.luid = 1;
    eth.name = "Ethernet";
    eth.operational = true;
    eth.loopback = false;
    eth.received_bytes = 3000;
    eth.sent_bytes = 1000;

    const PreviousBytes previous{{1, {1000, 0}}};

    auto interfaces = translate({eth}, previous, 1.0);

    CHECK_EQ(interfaces.size(), static_cast<std::size_t>(1));
    CHECK_EQ(interfaces[0].name, std::string("Ethernet"));
    CHECK(interfaces[0].receive_bytes_per_second == 2000.0);
    CHECK(interfaces[0].send_bytes_per_second == 1000.0);
}

TEST_CASE("network translate: loopback interfaces are dropped") {
    RawInterfaceSample loop;
    loop.luid = 2;
    loop.name = "Loopback Pseudo-Interface 1";
    loop.operational = true;
    loop.loopback = true;

    auto interfaces = translate({loop}, {}, 1.0);

    CHECK_EQ(interfaces.size(), static_cast<std::size_t>(0));
}

TEST_CASE("network translate: filter-driver shadow interfaces are dropped") {
    RawInterfaceSample filter_iface;
    filter_iface.luid = 5;
    filter_iface.name = "Wi-Fi-WFP Native MAC Layer LightWeight Filter-0000";
    filter_iface.operational = true;
    filter_iface.loopback = false;
    filter_iface.filter_interface = true;

    auto interfaces = translate({filter_iface}, {}, 1.0);

    CHECK_EQ(interfaces.size(), static_cast<std::size_t>(0));
}

TEST_CASE("network translate: non-operational interfaces are dropped") {
    RawInterfaceSample down;
    down.luid = 3;
    down.name = "Disabled NIC";
    down.operational = false;
    down.loopback = false;

    auto interfaces = translate({down}, {}, 1.0);

    CHECK_EQ(interfaces.size(), static_cast<std::size_t>(0));
}

TEST_CASE("network translate: an interface absent from the previous sample reports 0 bytes/sec") {
    RawInterfaceSample fresh;
    fresh.luid = 4;
    fresh.name = "New Adapter";
    fresh.operational = true;
    fresh.received_bytes = 5000;
    fresh.sent_bytes = 5000;

    auto interfaces = translate({fresh}, {}, 1.0); // empty previous_bytes

    CHECK_EQ(interfaces.size(), static_cast<std::size_t>(1));
    CHECK(interfaces[0].receive_bytes_per_second == 0.0);
    CHECK(interfaces[0].send_bytes_per_second == 0.0);
}

TEST_CASE("network translate: empty input yields an empty result") {
    auto interfaces = translate({}, {}, 1.0);
    CHECK_EQ(interfaces.size(), static_cast<std::size_t>(0));
}
