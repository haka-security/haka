
find_package(NetfilterQueue)
if(NETFILTERQUEUE_FOUND)
	add_library(packet-nfqueue MODULE
		main.c
		iptables.c)

	include_directories(${NETFILTERQUEUE_INCLUDE_DIR})
	target_link_libraries(packet-nfqueue ${NETFILTERQUEUE_LIBRARIES})
	
	INSTALL_MODULE(packet-nfqueue)
endif(NETFILTERQUEUE_FOUND)
