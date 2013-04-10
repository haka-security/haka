
find_package(NetfilterQueue)
if(NETFILTERQUEUE_FOUND)
	add_library(packet-nfqueue MODULE
		main.c
		iptables.c)
	target_link_libraries(packet-nfqueue libhaka)

	include_directories(${NETFILTERQUEUE_INCLUDE_DIR})
	target_link_libraries(packet-nfqueue ${NETFILTERQUEUE_LIBRARIES})

	install(TARGETS packet-nfqueue LIBRARY DESTINATION bin/modules)
endif(NETFILTERQUEUE_FOUND)
