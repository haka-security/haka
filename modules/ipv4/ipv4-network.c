#include "haka/ipv4.h"
#include "haka/ipv4-address.h"
#include "haka/ipv4-network.h"
#include <haka/log.h>
#include <haka/error.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IPV4_MASK_MAXVAL 32


void ipv4_network_to_string(ipv4network netaddr, char *string, size_t size)
{
	ipv4_addr_to_string(netaddr.net, string, IPV4_ADDR_STRING_MAXLEN);
	snprintf(string + strlen(string), IPV4_NET_STRING_MAXLEN - IPV4_ADDR_STRING_MAXLEN + 1, "/%hhu", netaddr.mask);
}

static bool ipv4_check_netmask(ipv4network *network)
{
	return ((network->net & ((1 << network->mask) - 1) << (IPV4_MASK_MAXVAL - network->mask))  == network->net);
}

ipv4network ipv4_network_from_string(const char *string)
{
	int8 * ptr;
	if (!(ptr = strchr(string, '/'))) {
		error(L"Unknown network format");
		return ipv4_network_zero;
	}

	int32 slash_index = ptr - string;
	char tmp[IPV4_NET_STRING_MAXLEN + 1];
	strncpy(tmp, string, IPV4_NET_STRING_MAXLEN);
	tmp[slash_index] = '\0';

	ipv4network netaddr;
	netaddr.net = ipv4_addr_from_string(tmp);

	if ((sscanf(string + slash_index, "/%hhu", &netaddr.mask) != 1) ||
		(netaddr.mask > 32 || netaddr.mask < 0)) {
		error(L"Unknown network format");
		return ipv4_network_zero;
	}

	if (!ipv4_check_netmask(&netaddr)) {
		netaddr.net = netaddr.net & (((1 << netaddr.mask) - 1) << (IPV4_MASK_MAXVAL - netaddr.mask));
		message(HAKA_LOG_WARNING, L"ipv4" , L"Incorrect Net/Mask values (fixed)");
	}
	return netaddr;
}

uint8 ipv4_check_addr_in_network(ipv4network network, ipv4addr addr)
{
    return ((addr & ((1 << network.mask) - 1) << (IPV4_MASK_MAXVAL - network.mask))  == network.net);
}

