# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)
include(TestUnit)

TEST_PCAP(ipv4 setbadfields)
TEST_PCAP(ipv4 hook-up-rule)
TEST_PCAP(ipv4 hook-down-rule)
TEST_PCAP(ipv4 land)
TEST_PCAP(ipv4 nat)
TEST_PCAP(ipv4 net)
TEST_PCAP(ipv4 getfields)
TEST_PCAP(ipv4 getfields-lua)
TEST_PCAP(ipv4 setfields)
TEST_PCAP(ipv4 setfields-lua)
TEST_PCAP(ipv4 getfields-passthrough OPTIONS --pass-through)
TEST_PCAP(ipv4 setfields-passthrough OPTIONS --pass-through)
TEST_PCAP(ipv4 create)
TEST_PCAP(ipv4 fragment)
TEST_PCAP(ipv4 fragment_overlap_1)
TEST_PCAP(ipv4 fragment_overlap_2)
TEST_PCAP(ipv4 fragment_overrun)
TEST_PCAP(ipv4 fragment_incomplete)
# TEST_PCAP(ipv4 options)

TEST_UNIT(MODULE ipv4 NAME unit FILES unit.c LIBS ipv4)
