/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

	lua_object_init(&ip->lua_object);
	ip->packet = packet;
	ip->header = (struct ipv4_header*)(packet_data(packet));
	ip->modified = false;
	ip->invalid_checksum = false;
	ip->drop = false;

	return ip;
}

struct ipv4 *ipv4_create(struct packet *packet)
{
	struct ipv4 *ip = NULL;

	assert(packet);

	ip = malloc(sizeof(struct ipv4));
	if (!ip) {
		return NULL;
	}

	lua_object_init(&ip->lua_object);
	ip->packet = packet;

	packet_resize(packet, sizeof(struct ipv4_header));
	ip->header = (struct ipv4_header*)(packet_data_modifiable(packet));
	if (!ip->header) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	ip->modified = true;
	ip->invalid_checksum = true;
	ip->drop = false;

	ipv4_set_version(ip, 4);
	ipv4_set_checksum(ip, 0);
	ipv4_set_len(ip, sizeof(struct ipv4_header));
	ipv4_set_hdr_len(ip, sizeof(struct ipv4_header));

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
			packet_release(packet);
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
		packet_release(packet);
	}
}

void ipv4_release(struct ipv4 *ip)
{
	lua_object_release(ip, &ip->lua_object);
	ipv4_flush(ip);
	free(ip);
}

int ipv4_pre_modify(struct ipv4 *ip)
{
	if (!ip->modified) {
		struct ipv4_header *header = (struct ipv4_header *)(packet_data_modifiable(ip->packet));
		if (!header) {
			assert(check_error());
			return -1;
		}

		ip->header = header;
	}

	ip->modified = true;
	return 0;
}

int ipv4_pre_modify_header(struct ipv4 *ip)
{
	const int ret = ipv4_pre_modify(ip);
	if (ret) return ret;

	ip->invalid_checksum = true;
	return 0;
}


/* compute tcp checksum RFC #1071 */
//TODO: To be optimized
int16 inet_checksum(uint16 *ptr, uint16 size)
{
	register long sum = 0;

	while (size > 1) {
		sum += *ptr++;
		size -= 2;
	}

	if (size > 0) {
#ifdef HAKA_LITTLEENDIAN
		sum += *(uint8 *)ptr;
#else
		sum += (*(uint8 *)ptr) << 8;
#endif
	}

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
	if (!ipv4_pre_modify_header(ip)) {
		ip->header->checksum = 0;
		ip->header->checksum = inet_checksum((uint16 *)ip->header, ipv4_get_hdr_len(ip));
		ip->invalid_checksum = false;
	}
}

const uint8 *ipv4_get_payload(struct ipv4 *ip)
{
	IPV4_CHECK(ip, NULL);
	return ((const uint8 *)ip->header) + ipv4_get_hdr_len(ip);
}

uint8 *ipv4_get_payload_modifiable(struct ipv4 *ip)
{
	IPV4_CHECK(ip, NULL);
	if (!ipv4_pre_modify(ip))
		return ((uint8 *)ip->header) + ipv4_get_hdr_len(ip);
	else
		return NULL;
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
	if (packet_resize(ip->packet, new_size)) {
		return NULL;
	}

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
	IPV4_CHECK(ip);
	ip->drop = true;
}

void ipv4_action_send(struct ipv4 *ip)
{
	struct packet *packet;

	IPV4_CHECK(ip);

	while ((packet = ipv4_forge(ip))) {
		packet_send(packet);
	}
}

bool ipv4_valid(struct ipv4 *ip)
{
	return !ip->drop;
}
