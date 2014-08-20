# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(PCAP REQUIRED)
if(PCAP_FOUND)
	INCLUDE_MODULE(pcap packet)

	add_library(memory MODULE main.c)
	set_target_properties(memory PROPERTIES OUTPUT_NAME memory)

	include_directories(${PCAP_INCLUDE_DIR})
	target_link_libraries(memory ${PCAP_LIBRARY})

	DEPENDS_MODULE(memory packet-pcap packet)
	INSTALL_MODULE(memory packet)

	# Test
	# include(TestModuleLoad)
	# TEST_MODULE_LOAD(packet-memory)
endif()
