
# Tests
include(TestPcap)

TEST_PCAP(core rule-error)
TEST_PCAP(core onexit)
TEST_PCAP(core timestamp)
TEST_PCAP(core state_machine)
TEST_PCAP(core gettingstarted CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/doc/gettingstarted.lua)
TEST_PCAP(core standard-http CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/standard/config.lua)
TEST_PCAP(core standard-ipv4 CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/standard/config.lua)
TEST_PCAP(core standard-tcp CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/standard/config.lua)
