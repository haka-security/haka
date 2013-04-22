
# Tests
include(TestPcapCompare)
include(TestPcapOutput)

TEST_PCAP_COMPARE(land)
TEST_PCAP_COMPARE(nat)
TEST_PCAP_OUTPUT(getfields)
TEST_PCAP_COMPARE(setfields)
