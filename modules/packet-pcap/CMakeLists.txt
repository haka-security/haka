
find_package(PCAP)
if(PCAP_FOUND)
	add_library(packet-pcap MODULE main.c)
	target_link_libraries(packet-pcap libhaka)

	include_directories(${PCAP_INCLUDE_DIR})
	target_link_libraries(packet-pcap ${PCAP_LIBRARY})

	install(TARGETS packet-pcap LIBRARY DESTINATION bin/modules)

	# Test
	include(TestModuleLoad)
	TEST_MODULE_LOAD(packet-pcap)
endif()

