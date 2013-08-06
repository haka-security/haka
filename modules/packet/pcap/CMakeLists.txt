
find_package(PCAP)
if(PCAP_FOUND)
	add_library(packet-pcap MODULE main.c)
	set_target_properties(packet-pcap PROPERTIES OUTPUT_NAME pcap)

	include_directories(${PCAP_INCLUDE_DIR})
	target_link_libraries(packet-pcap ${PCAP_LIBRARY})

	INSTALL_MODULE(packet-pcap packet)

	# Test
	# include(TestModuleLoad)
	# TEST_MODULE_LOAD(packet-pcap)
endif()
