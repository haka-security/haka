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
	struct vbuffer_sub header_part;
	size_t len;
	const uint8 *ptr;
	if (!vbuffer_sub_create(&header_part, payload, 0, hdrlen)) {
		return false;
	}

	ptr = vbuffer_sub_flatten(&header_part, &len);
	assert(len >= hdrlen);
	return ptr != NULL;
}

static bool ipv4_extract_payload(struct ipv4 *ip, size_t hdrlen, size_t size)
{
	struct vbuffer_sub header;

	/* extract the ip data, we cannot just take everything that is after the header
	 * as the packet might contains some padding.
	 */
	if (!vbuffer_sub_create(&header, &ip->packet->payload, hdrlen, size)) {
		assert(check_error());
		return false;
	}

	if (!vbuffer_select(&header, &ip->payload, &ip->select)) {
		assert(check_error());
		return false;
	}

	return true;
}

struct ipv4 *ipv4_dissect(struct packet *packet)
{
	struct ipv4 *ip = NULL;
	struct vbuffer *payload;
	struct vbuffer_iterator hdrleniter;
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

	if (!payload) {
		assert(check_error());
		return NULL;
	}

	if (!vbuffer_check_size(payload, sizeof(struct ipv4_header), NULL)) {
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

	/* extract ip header len */
	vbuffer_begin(payload, &hdrleniter);
	*(uint8 *)&hdrlen = vbuffer_iterator_getbyte(&hdrleniter);

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

	if (!ipv4_extract_payload(ip, hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET, ipv4_get_len(ip))) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	ip->lua_object = lua_object_init;
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
		struct vbuffer header_buffer;
		if (!vbuffer_create_new(&header_buffer, hdrlen, true)) {
			assert(check_error());
			free(ip);
			return NULL;
		}

		vbuffer_append(payload, &header_buffer);
	}

	if (!ipv4_extract_payload(ip, hdrlen, ALL)) {
		assert(check_error());
		free(ip);
		return NULL;
	}

	ipv4_set_version(ip, 4);
	ipv4_set_checksum(ip, 0);
	ipv4_set_len(ip, hdrlen);
	ipv4_set_hdr_len(ip, hdrlen);

	ip->lua_object = lua_object_init;
	return ip;
}

struct packet *ipv4_forge(struct ipv4 *ip)
{
	struct packet *packet = ip->packet;
	if (packet) {
		const size_t len = ipv4_get_hdr_len(ip) + vbuffer_size(&ip->payload);
		if (len != ipv4_get_len(ip)) {
			ipv4_set_len(ip, len);
		}

		if (ip->invalid_checksum)
			ipv4_compute_checksum(ip);

		vbuffer_restore(&ip->select, &ip->payload);

		vbuffer_clear(&ip->payload);
		ip->packet = NULL;
		return packet;
	}
	else {
		return NULL;
	}
}

struct ipv4_header *ipv4_header(struct ipv4 *ip, bool write)
{
	IPV4_CHECK(ip, NULL);
	struct vbuffer_iterator begin;
	struct ipv4_header *header;
	size_t len;

	if (!vbuffer_begin(&ip->packet->payload, &begin)) {
		return NULL;
	}

	header = (struct ipv4_header *)vbuffer_iterator_mmap(&begin, ALL, &len, write);
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
		vbuffer_clear(&ip->payload);
		ip->packet = NULL;
	}
}

void ipv4_release(struct ipv4 *ip)
{
	lua_object_release(ip, &ip->lua_object);
	ipv4_flush(ip);
	vbuffer_release(&ip->payload);
	free(ip);
}

/* compute tcp checksum RFC #1071 */
//TODO: To be optimized
int32 inet_checksum_partial(uint16 *ptr, size_t size, bool *odd)
{
	register long sum = 0;

	if (*odd) {
#ifdef HAKA_LITTLEENDIAN
		sum += (*(uint8 *)ptr) << 8;
#else
		sum += *(uint8 *)ptr;
#endif
		ptr = (uint16 *)(((uint8 *)ptr) + 1);
		--size;
	}

	while (size > 1) {
		sum += *ptr++;
		size -= 2;
	}

	*odd = size > 0;

	if (*odd) {
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
	bool odd = false;
	const int32 sum = inet_checksum_partial(ptr, size, &odd);
	return inet_checksum_reduce(sum);
}

int32 inet_checksum_vbuffer_partial(struct vbuffer_sub *buf, bool *odd)
{
	int32 sum = 0;
	uint8 *data;
	size_t len;
	struct vbuffer_sub_mmap iter = vbuffer_mmap_init;

	while ((data = vbuffer_mmap(buf, &len, false, &iter))) {
		if (len > 0) {
			sum += inet_checksum_partial((uint16 *)data, len, odd);
		}
	}

	return sum;
}

int16 inet_checksum_vbuffer(struct vbuffer_sub *buf)
{
	bool odd = false;
	const int32 sum = inet_checksum_vbuffer_partial(buf, &odd);
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
	return vbuffer_size(&ip->payload);
}

void ipv4_action_drop(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	ipv4_flush(ip);
}
