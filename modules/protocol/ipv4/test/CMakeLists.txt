
# Tests
include(TestPcap)
include(TestUnit)

TEST_PCAP(ipv4 setbadfields)
TEST_PCAP(ipv4 hook-up-rule)
TEST_PCAP(ipv4 hook-down-rule)
TEST_PCAP(ipv4 land)
TEST_PCAP(ipv4 nat)
TEST_PCAP(ipv4 net)
TEST_PCAP(ipv4 getfields)
TEST_PCAP(ipv4 setfields)
TEST_PCAP(ipv4 getfields-passthrough OPTIONS --pass-through)
TEST_PCAP(ipv4 setfields-passthrough OPTIONS --pass-through)
TEST_PCAP(ipv4 create)

TEST_UNIT(ipv4 unit unit.c)
target_link_libraries(ipv4-unit ipv4)
