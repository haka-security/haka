/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "haka/ipv4.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <haka/log.h>
#include <haka/alert.h>
#include <haka/error.h>
#include <haka/string.h>


static bool ipv4_flatten_header(struct vbuffer *payload, size_t hdrlen)
{
	struct vsubbuffer header_part;
	if (!vbuffer_sub(payload, 0, hdrlen, &header_part)) {
		return false;
	}

	if (!vsubbuffer_flatten(&header_part)) {
		return false;
	}

	return true;
}

struct ipv4 *ipv4_dissect(struct packet *packet)
{
	struct ipv4 *ip = NULL;
	struct vbuffer *payload;
	struct {
#ifdef HAKA_LITTLEENDIAN
		uint8    hdr_len:4;
		uint8    version:4;
#else
		uint8    version:4;
		uint8    hdr_len:4;
#endif
	} hdrlen;

	assert(packet);
	payload = packet_payload(packet);

	if (!vbuffer_checksize(payload, sizeof(struct ipv4_header))) {
		TOWSTR(srcip, ipv4addr, ipv4_get_src(ip));
		TOWSTR(dstip, ipv4addr, ipv4_get_dst(ip));
		ALERT(invalid_packet, 1, 1)
			description: L"invalid ip packet, size is too small",
			severity: HAKA_ALERT_LOW,
		ENDALERT

		ALERT_NODE(invalid_packet, sources, 0, HAKA_ALERT_NODE_ADDRESS, srcip);
		ALERT_NODE(invalid_packet, targets, 0, HAKA_ALERT_NODE_ADDRESS, dstip);

		alert(&invalid_packet);

		packet_drop(packet);
		packet_release(packet);
		return NULL;
	}

	ip = malloc(sizeof(struct ipv4));
	if (!ip) {
		error(L"memory error");
		return NULL;
	}

	ip->packet = packet;
	ip->invalid_checksum = false;

	if (!payload) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	/* extract ip header len */
	*(uint8 *)&hdrlen = vbuffer_getbyte(payload, 0);

	if (!ipv4_flatten_header(payload, hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET)) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	/* extract the ip data, we cannot just take everything that is after the header
	 * as the packet might contains some padding.
	 */
	if (vbuffer_size(payload) < ipv4_get_len(ip)) {
		TOWSTR(srcip, ipv4addr, ipv4_get_src(ip));
		TOWSTR(dstip, ipv4addr, ipv4_get_dst(ip));
		ALERT(invalid_packet, 1, 1)
			description: L"invalid ip packet, invalid size is too small",
			severity: HAKA_ALERT_LOW,
		ENDALERT

		ALERT_NODE(invalid_packet, sources, 0, HAKA_ALERT_NODE_ADDRESS, srcip);
		ALERT_NODE(invalid_packet, targets, 0, HAKA_ALERT_NODE_ADDRESS, dstip);

		alert(&invalid_packet);

		packet_drop(packet);
		packet_release(packet);
		free(ip);
		return NULL;
	}

	ip->payload = vbuffer_select(payload, hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET,
			ipv4_get_len(ip)-(hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET), &ip->select);
	if (!ip->payload) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	lua_object_init(&ip->lua_object);
	return ip;
}

struct ipv4 *ipv4_create(struct packet *packet)
{
	struct ipv4 *ip = NULL;
	struct vbuffer *payload;
	size_t hdrlen;

	assert(packet);

	ip = malloc(sizeof(struct ipv4));
	if (!ip) {
		return NULL;
	}

	ip->packet = packet;
	ip->invalid_checksum = true;

	payload = packet_payload(packet);

	hdrlen = sizeof(struct ipv4_header);

	{
		size_t len;
		uint8 *ptr;
		struct vbuffer *header_buffer = vbuffer_create_new(hdrlen);
		if (!header_buffer) {
			assert(check_error());
			free(ip);
			return NULL;
		}

		ptr = vbuffer_mmap(header_buffer, NULL, &len, true);
		assert(ptr);
		memset(ptr, 0, len);

		vbuffer_insert(payload, 0, header_buffer);
	}

	ip->payload = vbuffer_select(payload, hdrlen, ALL, &ip->select);
	if (!ip->payload) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	ipv4_set_version(ip, 4);
	ipv4_set_checksum(ip, 0);
	ipv4_set_len(ip, hdrlen);
	ipv4_set_hdr_len(ip, hdrlen);

	lua_object_init(&ip->lua_object);
	return ip;
}

struct packet *ipv4_forge(struct ipv4 *ip)
{
	struct packet *packet = ip->packet;
	if (packet) {
		const size_t len = ipv4_get_hdr_len(ip) + vbuffer_size(ip->payload);
		if (len != ipv4_get_len(ip)) {
			ipv4_set_len(ip, len);
		}

		if (ip->invalid_checksum)
			ipv4_compute_checksum(ip);

		vbuffer_restore(ip->select, ip->payload);
		ip->select = NULL;

		ip->payload = NULL;
		ip->packet = NULL;
		return packet;
	}
	else {
		return NULL;
	}
}

struct ipv4_header *ipv4_header(struct ipv4 *ip, bool write)
{
	struct ipv4_header *header;
	size_t len;
	header = (struct ipv4_header *)vbuffer_mmap(ip->packet->payload, NULL, &len, write);
	if (!header) {
		assert(write); /* should always work in read mode */
		assert(check_error());
		return NULL;
	}

	if (write) {
		ip->invalid_checksum = true;
	}

	assert(len >= sizeof(struct ipv4_header));
	return header;
}

static void ipv4_flush(struct ipv4 *ip)
{
	if (ip->packet) {
		packet_drop(ip->packet);
		packet_release(ip->packet);
		vbuffer_free(ip->payload);
		ip->payload = NULL;
		ip->packet = NULL;
	}
}

void ipv4_release(struct ipv4 *ip)
{
	lua_object_release(ip, &ip->lua_object);
	ipv4_flush(ip);
	free(ip);
}

/* compute tcp checksum RFC #1071 */
//TODO: To be optimized
int32 inet_checksum_partial(uint16 *ptr, size_t size)
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

	return sum;
}

int16 inet_checksum_reduce(int32 sum)
{
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

int16 inet_checksum(uint16 *ptr, size_t size)
{
	const int32 sum = inet_checksum_partial(ptr, size);
	return inet_checksum_reduce(sum);
}

int32 inet_checksum_vbuffer_partial(struct vsubbuffer *buf)
{
	int32 sum = 0;
	void *iter = NULL;
	uint8 *data;
	size_t len, remlen = 0;

	while ((data = vsubbuffer_mmap(buf, &iter, &remlen, &len, false))) {
		if (len > 0) {
			sum += inet_checksum_partial((uint16 *)data, len);
		}
	}

	return sum;
}

int16 inet_checksum_vbuffer(struct vsubbuffer *buf)
{
	const int32 sum = inet_checksum_vbuffer_partial(buf);
	return inet_checksum_reduce(sum);
}

bool ipv4_verify_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip, false);
	return inet_checksum((uint16 *)ipv4_header(ip, false), ipv4_get_hdr_len(ip)) == 0;
}

void ipv4_compute_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	struct ipv4_header *header = ipv4_header(ip, true);
	if (header) {
		header->checksum = 0;
		header->checksum = inet_checksum((uint16 *)header, ipv4_get_hdr_len(ip));
		ip->invalid_checksum = false;
	}
}

size_t ipv4_get_payload_length(struct ipv4 *ip)
{
	IPV4_CHECK(ip, 0);
	return vbuffer_size(ip->payload);
}

void ipv4_action_drop(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	ipv4_flush(ip);
}
