# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

#find_package(PCAP)
#if(PCAP_FOUND)
        add_library(packet-netmap MODULE main.c libnetmap.c)
	set_target_properties(packet-netmap PROPERTIES OUTPUT_NAME netmap)

	include_directories(.)
#	target_link_libraries(packet-pcap ${PCAP_LIBRARY})

	INSTALL_MODULE(packet-netmap packet)

	# Test
	# include(TestModuleLoad)
	# TEST_MODULE_LOAD(packet-pcap)
#endif()
