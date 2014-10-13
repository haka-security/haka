/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "haka/ipv4.h"

#include <haka/log.h>
#include <haka/error.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define IPV4_APPLY_NETMASK(addr, network)	\
	((addr) & ((1 << (network).mask) - 1) << (IPV4_MASK_MAXVAL - (network).mask))

static REGISTER_LOG_SECTION(ipv4);

const ipv4network ipv4_network_zero = {
	net:  0,
	mask: 0
};

void ipv4_network_to_string(ipv4network netaddr, char *string, size_t size)
{
	int length;
	ipv4_addr_to_string(netaddr.net, string, size);
	length = strlen(string);
	assert(length <= size);
	snprintf(string + length, size - length, "/%hhu", netaddr.mask);
}

ipv4network ipv4_network_from_string(const char *string)
{
	int8 * ptr;
	if (!(ptr = strchr(string, '/'))) {
		error("invalid IPv4 network address format");
		return ipv4_network_zero;
	}

	int32 slash_index = ptr - string;
	if (slash_index > IPV4_ADDR_STRING_MAXLEN) {
		error("invalid IPv4 network address format");
		return ipv4_network_zero;
	}

	char tmp[IPV4_ADDR_STRING_MAXLEN+1];
	strncpy(tmp, string, slash_index);
	tmp[slash_index] = '\0';

	ipv4network netaddr;
	netaddr.net = ipv4_addr_from_string(tmp);
	if (check_error()) {
		return ipv4_network_zero;
	}

	if ((sscanf(string + slash_index, "/%hhu", &netaddr.mask) != 1) ||
		(netaddr.mask > 32 || netaddr.mask < 0)) {
		error("invalid IPv4 network address format");
		return ipv4_network_zero;
	}

	ipv4addr maskedaddr;
	maskedaddr = IPV4_APPLY_NETMASK(netaddr.net, netaddr);

	if (maskedaddr != netaddr.net) {
		/* incorrect network mask, correct it */
		netaddr.net = maskedaddr;
		LOG_DEBUG(ipv4, "incorrect network mask");
	}

	return netaddr;
}

uint8 ipv4_network_contains(ipv4network network, ipv4addr addr)
{
	return (IPV4_APPLY_NETMASK(addr, network) == network.net);
}
