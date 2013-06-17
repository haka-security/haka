# Tests
include(TestPcap)
include(TestUnit)

TEST_PCAP(tcp getfields)
TEST_PCAP(tcp filter)
TEST_PCAP(tcp setchecksum)
TEST_PCAP(tcp setfields)

TEST_PCAP(tcp oneconnection)
TEST_PCAP(tcp interleavedconnection)

TEST_PCAP(tcp streamread)
TEST_PCAP(tcp streamdelete)
TEST_PCAP(tcp streaminsert)

TEST_UNIT(tcp unit unit.c)
target_link_libraries(tcp-unit tcp)
