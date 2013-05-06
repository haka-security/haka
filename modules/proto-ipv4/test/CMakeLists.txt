
# Tests
include(TestPcapCompare)
include(TestPcapOutput)
include(TestUnit)

TEST_PCAP_COMPARE(land)
TEST_PCAP_COMPARE(nat)
TEST_PCAP_OUTPUT(getfields)
TEST_PCAP_COMPARE(setfields)

TEST_UNIT(unit-ipv4 unit.c)
target_link_libraries(unit-ipv4 proto-ipv4)
