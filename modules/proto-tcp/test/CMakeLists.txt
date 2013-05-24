# Tests
include(TestPcapCompare)
include(TestPcapOutput)
include(TestUnit)

TEST_PCAP_OUTPUT(gettcpfields)
TEST_PCAP_COMPARE(tcpfilter)
TEST_PCAP_COMPARE(setchecksum)
TEST_PCAP_COMPARE(settcpfields)

TEST_PCAP_OUTPUT(oneconnection)
TEST_PCAP_OUTPUT(interleavedconnection)

TEST_UNIT(tcp-unit unit.c)
target_link_libraries(tcp-unit proto-tcp)
