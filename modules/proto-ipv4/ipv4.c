
#include "haka/ipv4.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <haka/log.h>
#include <haka/error.h>


struct ipv4 *ipv4_dissect(struct packet* packet)
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

	return ip;
}

void ipv4_release(struct ipv4 *ip)
{
	free(ip);
}

void ipv4_forge(struct ipv4 *ip)
{
	if (ip->invalid_checksum)
		ipv4_compute_checksum(ip);
}

void ipv4_pre_modify(struct ipv4 *ip)
{
	if (!ip->modified) {
		ip->header = (struct ipv4_header*)(packet_make_modifiable(ip->packet));
	}

	ip->modified = true;
}

void ipv4_pre_modify_header(struct ipv4 *ip)
{
	ipv4_pre_modify(ip);
	ip->invalid_checksum = true;
}

/* compute checksum RFC #1071 */
int16 checksum(const struct ipv4 *ip)
{
	register long sum = 0;
	uint16 size = ipv4_get_hdr_len(ip);
	uint16 *ptr = (uint16 *)ip->header;

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
	return checksum(ip) == 0;
}

void ipv4_compute_checksum(struct ipv4 *ip)
{
	ipv4_pre_modify_header(ip);

	ip->header->checksum = 0;
	ip->header->checksum = checksum(ip);
	ip->invalid_checksum = false;
}

void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size)
{
	uint8* bytes = (uint8*)&addr;
#if HAKA_LITTLEENDIAN
	snprintf(string, size, "%hhu.%hhu.%hhu.%hhu", bytes[3], bytes[2], bytes[1], bytes[0]);
#else
	snprintf(string, size, "%hhu.%hhu.%hhu.%hhu", bytes[0], bytes[1], bytes[2], bytes[3]);
#endif
}

uint32 ipv4_addr_from_string(const char *string)
{
	ipv4addr addr;
	uint8* bytes = (uint8*)&addr;

#if HAKA_LITTLEENDIAN
	if (sscanf(string, "%hhu.%hhu.%hhu.%hhu", bytes+3, bytes+2, bytes+1, bytes) != 4) {
#else
	if (sscanf(string, "%hhu.%hhu.%hhu.%hhu", bytes, bytes+1, bytes+2, bytes+3) != 4) {
#endif
		return 0;
	}

	return addr;
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
	return ((const uint8 *)ip->header) + ipv4_get_hdr_len(ip)*4;
}

uint8 *ipv4_get_payload_modifiable(struct ipv4 *ip)
{
	ipv4_pre_modify(ip);
	return ((uint8 *)ip->header) + ipv4_get_hdr_len(ip)*4;
}
