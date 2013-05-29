# Tests
include(TestPcapCompare)
include(TestPcapOutput)
include(TestUnit)

TEST_PCAP_OUTPUT(tcp getfields)
TEST_PCAP_COMPARE(tcp filter)
TEST_PCAP_COMPARE(tcp setchecksum)
TEST_PCAP_COMPARE(tcp setfields)

TEST_PCAP_OUTPUT(tcp oneconnection)
TEST_PCAP_OUTPUT(tcp interleavedconnection)

TEST_UNIT(tcp unit unit.c)
target_link_libraries(tcp-unit tcp)
