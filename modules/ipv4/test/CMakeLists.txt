
# Tests
include(TestPcapCompare)
include(TestPcapOutput)
include(TestUnit)

TEST_PCAP_COMPARE(ipv4 land)
TEST_PCAP_COMPARE(ipv4 nat)
TEST_PCAP_OUTPUT(ipv4 getfields)
TEST_PCAP_OUTPUT(ipva4 net)
TEST_PCAP_COMPARE(ipv4 setfields)

TEST_UNIT(ipv4 unit unit.c)
target_link_libraries(ipv4-unit ipv4)
