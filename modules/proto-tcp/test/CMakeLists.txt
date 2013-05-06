# Tests
include(TestPcapCompare)
include(TestPcapOutput)

TEST_PCAP_OUTPUT(gettcpfields)
TEST_PCAP_COMPARE(tcpfilter)
TEST_PCAP_COMPARE(setchecksum)
TEST_PCAP_COMPARE(settcpfields)

TEST_PCAP_OUTPUT(oneconnection)
TEST_PCAP_OUTPUT(interleavedconnection)
