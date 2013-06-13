
find_package(NetfilterQueue)
find_package(PCAP)
if(NETFILTERQUEUE_FOUND AND PCAP_FOUND)
	add_library(packet-nfqueue MODULE
		main.c
		iptables.c)

	include_directories(${NETFILTERQUEUE_INCLUDE_DIR} ${PCAP_INCLUDE_DIR})
	target_link_libraries(packet-nfqueue ${NETFILTERQUEUE_LIBRARIES} ${PCAP_LIBRARY})
	
	INSTALL_MODULE(packet-nfqueue)
endif()
