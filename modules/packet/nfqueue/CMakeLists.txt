# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_package(NetfilterQueue)
find_package(PCAP)

include(CheckCSourceCompiles)

set(IPTABLES_PATH "/sbin" CACHE STRING "path to iptables binaries")

if(NETFILTERQUEUE_FOUND AND PCAP_FOUND)
	# Check if nfq_get_payload does take an argument as unisgned char** or char**
	set(CMAKE_REQUIRED_LIBRARIES ${NETFILTERQUEUE_LIBRARIES})
	set(CMAKE_REQUIRED_INCLUDES ${NETFILTERQUEUE_INCLUDE_DIR})
	CHECK_C_SOURCE_COMPILES("
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
int main()
{
	struct nfq_data *nfad = NULL;
	unsigned char *data = NULL;
	nfq_get_payload(nfad, &data);
	return 0;
}
" NFQ_GET_PAYLOAD_UNSIGNED_CHAR)

	add_library(packet-nfqueue MODULE
		main.c
		iptables.c)
	set_target_properties(packet-nfqueue PROPERTIES OUTPUT_NAME nfqueue)
	set_target_properties(packet-nfqueue PROPERTIES COMPILE_DEFINITIONS IPTABLES_PATH="${IPTABLES_PATH}")

	include_directories(${NETFILTERQUEUE_INCLUDE_DIR} ${PCAP_INCLUDE_DIR})
	target_link_libraries(packet-nfqueue ${NETFILTERQUEUE_LIBRARIES} ${PCAP_LIBRARY})

	if(NFQ_GET_PAYLOAD_UNSIGNED_CHAR)
		set_property(TARGET packet-nfqueue APPEND PROPERTY COMPILE_DEFINITIONS NFQ_GET_PAYLOAD_UNSIGNED_CHAR)
	endif(NFQ_GET_PAYLOAD_UNSIGNED_CHAR)

	INSTALL_MODULE(packet-nfqueue packet)
else()
    message(STATUS "Not building module nfqueue (missing libraries)")
endif()
