# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)
include(TestUnitLua)

TEST_PCAP(doc doc-alert CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/../ref/alert-test.lua)
TEST_UNIT_LUA(MODULE core NAME doc-regexp FILES ../ref/regexp.lua)

# Workshop
TEST_PCAP(doc workshop-hellopacket)
TEST_PCAP(doc workshop-ipfilter)
TEST_PCAP(doc workshop-tcpfilter)
TEST_PCAP(doc workshop-blurring-the-web)
TEST_PCAP(doc workshop-smtp_partial CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/../user/workshop/smtp_test.lua)
TEST_PCAP(doc workshop-smtp_final)
TEST_PCAP(doc workshop-smtp-spam_filter)
TEST_PCAP(doc workshop-smtp-mail_content)
