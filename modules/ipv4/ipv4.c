
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

struct packet *ipv4_forge(struct ipv4 *ip)
{
	struct packet *packet = ip->packet;
	if (packet) {
		if (ip->drop) {
			ip->packet = NULL;
			ip->header = NULL;
			packet_drop(packet);
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

static void ipv4_flush(struct ipv4 *ip)
{
	struct packet *packet;
	while ((packet = ipv4_forge(ip))) {
		packet_drop(packet);
	}
}

void ipv4_release(struct ipv4 *ip)
{
	ipv4_flush(ip);
	free(ip);
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

uint8 *ipv4_resize_payload(struct ipv4 *ip, size_t size)
{
	size_t new_size;

	IPV4_CHECK(ip, NULL);

	new_size = size + ipv4_get_hdr_len(ip);
	packet_resize(ip->packet, new_size);
	ip->header = (struct ipv4_header*)packet_data_modifiable(ip->packet);
	ip->modified = true;
	ip->invalid_checksum = true;
	ipv4_set_len(ip, new_size);
	return ((uint8 *)ip->header) + ipv4_get_hdr_len(ip);
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
