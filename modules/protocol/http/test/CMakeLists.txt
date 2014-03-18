# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Tests
include(TestPcap)
include(TestUnitLua)

TEST_PCAP(http request-response)
TEST_PCAP(http keepalive)
TEST_PCAP(http split-request)
TEST_PCAP(http bad-request)
TEST_PCAP(http request-modif)
TEST_PCAP(http response-modif)
TEST_PCAP(http passthrough OPTIONS --pass-through)
TEST_PCAP(http request-reset)
TEST_PCAP(http getiptcpfields)
TEST_PCAP(http datablob)
TEST_PCAP(http nullbytes)
TEST_PCAP(http binary_uri)
TEST_PCAP(http binary_header)
TEST_PCAP(http long_uri)
TEST_PCAP(http long_header)
TEST_PCAP(http long_header_key)
TEST_PCAP(http uri-split-pcap)
TEST_PCAP(http response-data)
TEST_PCAP(http response-data-modif)
TEST_UNIT_LUA(MODULE http NAME uri-normalize FILES uri-normalize)
TEST_UNIT_LUA(MODULE http NAME uri-split FILES uri-split)
