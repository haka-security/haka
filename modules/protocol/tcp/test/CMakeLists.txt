# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)
include(TestUnit)

TEST_PCAP(tcp hook-up-rule)
TEST_PCAP(tcp hook-down-rule)
TEST_PCAP(tcp ipv4-field-access)
TEST_PCAP(tcp getfields)
TEST_PCAP(tcp getfields-passthrough OPTIONS --pass-through)
TEST_PCAP(tcp filter)
TEST_PCAP(tcp setchecksum)
TEST_PCAP(tcp setfields)
TEST_PCAP(tcp setfields-passthrough OPTIONS --pass-through)

TEST_PCAP(tcp oneconnection)
TEST_PCAP(tcp interleavedconnection)

TEST_PCAP(tcp streamread)
TEST_PCAP(tcp streamdelete)
TEST_PCAP(tcp streaminsert)
TEST_PCAP(tcp streaminsert-passthrough OPTIONS --pass-through)
TEST_PCAP(tcp streamread-passthrough OPTIONS --pass-through)
TEST_PCAP(tcp streamerase-passthrough OPTIONS --pass-through)
TEST_PCAP(tcp stream-rst)
TEST_PCAP(tcp streammodif-rst)

TEST_PCAP(tcp create)
