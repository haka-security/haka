#include "haka/ipv4.h"
#include "haka/ipv4-address.h"
#include "haka/ipv4-network.h"

#include <stdlib.h>
#include <stdio.h>

#include <haka/log.h>
#include <haka/error.h>

#define IPV4_MASK_MAXVAL 32


void ipv4_network_to_string(ipv4network netaddr, char *string, size_t size)
{
    int32 size_cp;
    ipv4_addr_to_string_sized(netaddr.net, string, IPV4_ADDR_STRING_MAXLEN, &size_cp);

    snprintf(string + size_cp, size - size_cp, "/%hhu", netaddr.mask);
}

static bool ipv4_check_netmask(ipv4network *network)
{
    return ((network->net & ((1 << network->mask) - 1) << (IPV4_MASK_MAXVAL - network->mask))  == network->net);
}

ipv4network ipv4_network_from_string(const char *string)
{
    int32 size_cp;
    ipv4network netaddr;
    netaddr.net = ipv4_addr_from_string_sized(string, &size_cp);
    if (sscanf(string + size_cp, "/%hhu", &netaddr.mask) != 1) {
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

