# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(PCAP REQUIRED)
if(PCAP_FOUND)
	add_library(packet-memory MODULE main.c)
	set_target_properties(packet-memory PROPERTIES OUTPUT_NAME memory)

	include_directories(${PCAP_INCLUDE_DIR})
	target_link_libraries(packet-memory ${PCAP_LIBRARY})

	INSTALL_MODULE(packet-memory packet)

	# Test
	# include(TestModuleLoad)
	# TEST_MODULE_LOAD(packet-memory)
endif()
