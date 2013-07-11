# Tests
include(TestPcap)

TEST_PCAP(http request-response)
TEST_PCAP(http keepalive)
TEST_PCAP(http split-request)
TEST_PCAP(http bad-request)
TEST_PCAP(http request-modif)
TEST_PCAP(http response-modif)
TEST_PCAP(http passthrough OPTIONS --pass-through)
