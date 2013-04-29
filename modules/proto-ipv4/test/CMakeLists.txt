
# Tests
include(TestPcapCompare)
include(TestPcapOutput)

TEST_PCAP_COMPARE(land)
TEST_PCAP_COMPARE(nat)
TEST_PCAP_OUTPUT(getfields)
TEST_PCAP_COMPARE(setfields)

add_executable(ipv4-unit unit.c)
target_link_libraries(ipv4-unit proto-ipv4 ${CHECK_LIBRARIES})
add_test(ipv4-unit ${CMAKE_CURRENT_BINARY_DIR}/ipv4-unit)
