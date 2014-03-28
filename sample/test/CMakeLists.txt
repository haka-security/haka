# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)

TEST_PCAP(sample gettingstarted CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/gettingstarted/gettingstarted.lua)
TEST_PCAP(sample standard-http CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/ruleset/config.lua)
TEST_PCAP(sample standard-ipv4 CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/ruleset/config.lua)
TEST_PCAP(sample standard-tcp CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/ruleset/config.lua)
TEST_PCAP(sample hellopacket CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/hellopacket/hellopacket.lua)
TEST_PCAP(sample sqli-simple CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/sqli/sqli-simple.lua)
TEST_PCAP(sample sqli-decode CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/sqli/sqli-decode.lua)
TEST_PCAP(sample sqli-fine-grained CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/sqli/sqli-fine-grained.lua)
TEST_PCAP(sample sqli-groups CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/sqli/sqli-groups.lua)
TEST_PCAP(sample sqli-white-list CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/sqli/sqli-white-list.lua)
TEST_PCAP(sample ipfilter CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/filter/ipfilter.lua)
TEST_PCAP(sample groupfilter CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/filter/groupfilter.lua)
TEST_PCAP(sample httpfilter CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/filter/httpfilter.lua)
TEST_PCAP(sample httpmodif CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/filter/httpmodif.lua)
TEST_PCAP(sample tcpfilter CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/filter/tcpfilter.lua)
TEST_PCAP(sample stats_on_exit CONFIG ${TEST_RUNDIR}/${HAKA_INSTALL_PREFIX}/share/haka/sample/stats/stats_on_exit.lua)
