
# Tests
include(TestPcapCompare)
include(TestPcapOutput)

TEST_PCAP_COMPARE(pcap basic)
TEST_PCAP_OUTPUT(pcap output)
