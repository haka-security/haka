#include "haka/ipv4.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <haka/log.h>
#include <haka/error.h>


struct ipv4 *ipv4_dissect(struct packet *packet)
{
	struct ipv4 *ip = NULL;

	assert(packet);

	if (packet_length(packet) < sizeof(struct ipv4_header)) {
		return NULL;
	}

	ip = malloc(sizeof(struct ipv4));
	if (!ip) {
		return NULL;
	}

	ip->packet = packet;
	ip->header = (struct ipv4_header*)(packet_data(packet));
	ip->modified = false;
	ip->invalid_checksum = false;
	ip->drop = false;

	return ip;
}

void ipv4_release(struct ipv4 *ip)
{
	free(ip);
}

struct packet *ipv4_forge(struct ipv4 *ip)
{
	struct packet *packet = ip->packet;
	if (packet) {
		if (ip->drop) {
			packet_drop(packet);
			ip->packet = NULL;
			ip->header = NULL;
			return NULL;
		}
		else {
			if (ip->invalid_checksum)
				ipv4_compute_checksum(ip);

			ip->packet = NULL;
			ip->header = NULL;
			return packet;
		}
	}
	else {
		return NULL;
	}
}

void ipv4_pre_modify(struct ipv4 *ip)
{
	if (!ip->modified) {
		ip->header = (struct ipv4_header*)(packet_data_modifiable(ip->packet));
	}

	ip->modified = true;
}

void ipv4_pre_modify_header(struct ipv4 *ip)
{
	ipv4_pre_modify(ip);
	ip->invalid_checksum = true;
}


/* compute tcp checksum RFC #1071 */
int16 inet_checksum(uint16 *ptr, uint16 size)
{
	register long sum = 0;

	while (size > 1) {
		sum += *ptr++;
		size -= 2;
	}

	if (size > 0)
		sum += *(uint8 *)ptr;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}


bool ipv4_verify_checksum(const struct ipv4 *ip)
{
	IPV4_CHECK(ip, false);
	return inet_checksum((uint16 *)ip->header, ipv4_get_hdr_len(ip)) == 0;
}

void ipv4_compute_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	ipv4_pre_modify_header(ip);

	ip->header->checksum = 0;
	ip->header->checksum = inet_checksum((uint16 *)ip->header, ipv4_get_hdr_len(ip));
	ip->invalid_checksum = false;
}


static void ipv4_addr_to_string_sized(ipv4addr addr, char *string, size_t size, int32 *size_cp)
{
	uint8* bytes = (uint8*)&addr;
	if (size_cp) {
#if HAKA_LITTLEENDIAN
		snprintf(string, size, "%hhu.%hhu.%hhu.%hhu%n", bytes[3], bytes[2], bytes[1], bytes[0], size_cp);
#else
		snprintf(string, size, "%hhu.%hhu.%hhu.%hhu%n", bytes[0], bytes[1], bytes[2], bytes[3], size_cp);
#endif
	}
	else {
#if HAKA_LITTLEENDIAN
		snprintf(string, size, "%hhu.%hhu.%hhu.%hhu", bytes[3], bytes[2], bytes[1], bytes[0]);
#else
		snprintf(string, size, "%hhu.%hhu.%hhu.%hhu", bytes[0], bytes[1], bytes[2], bytes[3]);
#endif
	}
}

static uint32 ipv4_addr_from_string_sized(const char *string, int32 *size_cp)
{
	ipv4addr addr;
	uint8* bytes = (uint8*)&addr;
	if (size_cp) {
#if HAKA_LITTLEENDIAN
		if (sscanf(string, "%hhu.%hhu.%hhu.%hhu%n", bytes+3, bytes+2, bytes+1, bytes, size_cp) != 4) {
#else
		if (sscanf(string, "%hhu.%hhu.%hhu.%hhu%n", bytes, bytes+1, bytes+2, bytes+3, size_cp) != 4) {
#endif
			return 0;
		}
	}
	else {
#if HAKA_LITTLEENDIAN
		if (sscanf(string, "%hhu.%hhu.%hhu.%hhu", bytes+3, bytes+2, bytes+1, bytes) != 4) {
#else
		if (sscanf(string, "%hhu.%hhu.%hhu.%hhu", bytes, bytes+1, bytes+2, bytes+3) != 4) {
#endif
			return 0;
		}
	}
	return addr;
}


uint32 ipv4_addr_from_string(const char *string)
{
	return ipv4_addr_from_string_sized(string, NULL);
}

void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size)
{
	return ipv4_addr_to_string_sized(addr, string, size, NULL);
}


void ipv4_network_to_string(ipv4network netaddr, char *string, size_t size)
{
	int32 size_cp;
	ipv4_addr_to_string_sized(netaddr.net, string, IPV4_ADDR_STRING_MAXLEN, &size_cp);

	snprintf(string + size_cp, size_cp, "/%hhu", netaddr.mask);
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
	sscanf(string + size_cp, "/%hhu", &netaddr.mask);
	if (!ipv4_check_netmask(&netaddr)) {
		netaddr.net = netaddr.net & (((1 << netaddr.mask) - 1) << (IPV4_MASK_MAXVAL - netaddr.mask));
		message(HAKA_LOG_WARNING, L"filter" , L"Incorrect Net/Mask values (fixed)");
	}
	return netaddr;
}

ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d)
{
	ipv4addr addr;
	uint8* bytes = (uint8*)&addr;

#if HAKA_LITTLEENDIAN
	bytes[3] = a; bytes[2] = b; bytes[1] = c; bytes[0] = d;
#else
	bytes[0] = a; bytes[1] = b; bytes[2] = c; bytes[3] = d;
#endif

	return addr;
}

const uint8 *ipv4_get_payload(struct ipv4 *ip)
{
	IPV4_CHECK(ip, NULL);
	return ((const uint8 *)ip->header) + ipv4_get_hdr_len(ip);
}

uint8 *ipv4_get_payload_modifiable(struct ipv4 *ip)
{
	IPV4_CHECK(ip, NULL);
	ipv4_pre_modify(ip);
	return ((uint8 *)ip->header) + ipv4_get_hdr_len(ip);
}

size_t ipv4_get_payload_length(struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	const uint8 hdr_len = ipv4_get_hdr_len(ip);
	const size_t total_len = ipv4_get_len(ip);
	return total_len - hdr_len;
}

static char *ipv4_proto_dissector[256];

void ipv4_register_proto_dissector(uint8 proto, const char *dissector)
{
	if (ipv4_proto_dissector[proto] != NULL) {
		if (strcmp(ipv4_proto_dissector[proto], dissector) == 0) {
			return;
		}
		else {
			error(L"IPv4 protocol %d dissector already registered", proto);
			return;
		}
	}

	ipv4_proto_dissector[proto] = strdup(dissector);
}

const char *ipv4_get_proto_dissector(struct ipv4 *ip)
{
	IPV4_CHECK(ip, NULL);
	return ipv4_proto_dissector[ipv4_get_proto(ip)];
}

static FINI void ipv4_dissector_cleanup()
{
	int i;
	for (i = 0; i < 256; ++i) {
		free(ipv4_proto_dissector[i]);
		ipv4_proto_dissector[i] = NULL;
	}
}

void ipv4_action_drop(struct ipv4 *ip)
{
	ip->drop = true;
}

bool ipv4_valid(struct ipv4 *ip)
{
	return !ip->drop;
}
