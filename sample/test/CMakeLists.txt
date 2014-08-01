# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)

TEST_PCAP(sample gettingstarted CONFIG ${TEST_RUNDIR}/share/haka/sample/gettingstarted/gettingstarted.lua)
TEST_PCAP(sample standard-http CONFIG ${TEST_RUNDIR}/share/haka/sample/ruleset/config.lua)
TEST_PCAP(sample standard-ipv4 CONFIG ${TEST_RUNDIR}/share/haka/sample/ruleset/config.lua)
TEST_PCAP(sample standard-tcp CONFIG ${TEST_RUNDIR}/share/haka/sample/ruleset/config.lua)
TEST_PCAP(sample hellopacket CONFIG ${TEST_RUNDIR}/share/haka/sample/hellopacket/hellopacket.lua)
TEST_PCAP(sample sqli-simple CONFIG ${TEST_RUNDIR}/share/haka/sample/sqli/sqli-simple.lua)
TEST_PCAP(sample sqli-decode CONFIG ${TEST_RUNDIR}/share/haka/sample/sqli/sqli-decode.lua)
TEST_PCAP(sample sqli-fine-grained CONFIG ${TEST_RUNDIR}/share/haka/sample/sqli/sqli-fine-grained.lua)
TEST_PCAP(sample sqli-groups CONFIG ${TEST_RUNDIR}/share/haka/sample/sqli/sqli-groups.lua)
TEST_PCAP(sample sqli-white-list CONFIG ${TEST_RUNDIR}/share/haka/sample/sqli/sqli-white-list.lua)
TEST_PCAP(sample ipfilter CONFIG ${TEST_RUNDIR}/share/haka/sample/filter/ipfilter.lua)
TEST_PCAP(sample groupfilter CONFIG ${TEST_RUNDIR}/share/haka/sample/filter/groupfilter.lua)
TEST_PCAP(sample httpfilter CONFIG ${TEST_RUNDIR}/share/haka/sample/filter/httpfilter.lua)
TEST_PCAP(sample httpmodif CONFIG ${TEST_RUNDIR}/share/haka/sample/filter/httpmodif.lua)
TEST_PCAP(sample tcpfilter CONFIG ${TEST_RUNDIR}/share/haka/sample/filter/tcpfilter.lua)
TEST_PCAP(sample stats_on_exit CONFIG ${TEST_RUNDIR}/share/haka/sample/stats/stats_on_exit.lua)
TEST_PCAP(sample pdns CONFIG ${TEST_RUNDIR}/share/haka/sample/ruleset/dns/pdns.lua)
TEST_PCAP(sample smtp_spam_filter CONFIG ${TEST_RUNDIR}/share/haka/sample/smtp_dissector/smtp_spam_filter.lua)
TEST_PCAP(sample smtp_mail_content CONFIG ${TEST_RUNDIR}/share/haka/sample/smtp_dissector/smtp_mail_content.lua)
TEST_PCAP(sample smtp_command_response CONFIG ${TEST_RUNDIR}/share/haka/sample/smtp_dissector/smtp_command_response.lua)

# Module tutorial
add_test(NAME sample-mymodule-build COMMAND ${CMAKE_COMMAND}
	-DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
	-DHAKA_PATH=${TEST_RUNDIR}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/mymodule-build.cmake
)
TEST_PCAP(sample mymodule-run CONFIG ${TEST_RUNDIR}/share/haka/sample/mymodule/rule.lua)
SET_TESTS_PROPERTIES(sample-mymodule-run-pcap PROPERTIES DEPENDS sample-mymodule-build)
