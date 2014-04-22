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

	vbuffer_sub_create(&header_part, payload, 0, hdrlen);

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
	vbuffer_sub_create(&header, &ip->packet->payload, hdrlen, size);

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
	size_t header_len;

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

	header_len = hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET;

	if (!ipv4_flatten_header(payload, header_len)) {
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

	if (!ipv4_extract_payload(ip, header_len, ipv4_get_len(ip) - header_len)) {
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
		vbuffer_release(&header_buffer);
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

		vbuffer_restore(&ip->select, &ip->payload, false);

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

	vbuffer_begin(&ip->packet->payload, &begin);

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

/* compute inet checksum RFC #1071 */

struct checksum_partial checksum_partial_init = { false, 0, 0 };

#define REDUCE_DECL \
	union { \
		uint16  s[2]; \
		uint32  l; \
	} reduce_util;

typedef union {
	uint8   c[2];
	uint16  s;
} swap_util_t;

#define ADD_CARRY(x) (x > 0xffffUL ? x -= 0xffffUL : x)
#define REDUCE       { reduce_util.l = sum; sum = reduce_util.s[0] + reduce_util.s[1]; ADD_CARRY(sum); }

void inet_checksum_partial(struct checksum_partial *csum, const uint8 *ptr, size_t size)
{
	register int sum = csum->csum;
	register int len = size;
	register uint16 *w;
	int byte_swapped = 0;
	REDUCE_DECL;
	swap_util_t swap_util;

	if (csum->odd) {
		/* The last partial checksum len was not even. We need to take
		 * the leftover char into account.
		 */
		swap_util.c[0] = csum->leftover;
		swap_util.c[1] = *ptr++;
		sum += swap_util.s;
		len--;
	}

	/* Make sure that the pointer is aligned on 16 bit boundary */
	if ((1 & (ptrdiff_t)ptr) && (len > 0)) {
		REDUCE;
		sum <<= 8;
		swap_util.c[0] = *ptr++;
		len--;
		byte_swapped = 1;
	}

	w = (uint16 *)ptr;

	/* Unrolled loop */
	while ((len -= 32) >= 0) {
		sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3];
		sum += w[4]; sum += w[5]; sum += w[6]; sum += w[7];
		sum += w[8]; sum += w[9]; sum += w[10]; sum += w[11];
		sum += w[12]; sum += w[13]; sum += w[14]; sum += w[15];
		w += 16;
	}
	len += 32;

	while ((len -= 8) >= 0) {
		sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3];
		w += 4;
	}
	len += 8;

	if (len != 0 || byte_swapped) {
		REDUCE;
		while ((len -= 2) >= 0) {
			sum += *w++;
		}

		if (byte_swapped) {
			REDUCE;
			sum <<= 8;

			if (len == -1) {
				swap_util.c[1] = *(uint8 *)w;
				sum += swap_util.s;
				len = 0;
			} else {
				csum->leftover = swap_util.c[0];
				len = -1;
			}
		} else if (len == -1) {
			csum->leftover = *(uint8 *)w;
		}
	}

	/* Update csum context */
	csum->odd = (len == -1);
	csum->csum = sum;
}

int16 inet_checksum_reduce(struct checksum_partial *csum)
{
	register int32 sum = csum->csum;
	REDUCE_DECL;

	if (csum->odd) {
		swap_util_t swap_util;
		swap_util.c[0] = csum->leftover;
		swap_util.c[1] = 0;
		sum += swap_util.s;
	}

	REDUCE;
	return (~sum & 0xffff);
}

int16 inet_checksum(const uint8 *ptr, size_t size)
{
	struct checksum_partial csum = checksum_partial_init;
	inet_checksum_partial(&csum, ptr, size);
	return inet_checksum_reduce(&csum);
}

void inet_checksum_vbuffer_partial(struct checksum_partial *csum, struct vbuffer_sub *buf)
{
	struct vbuffer_sub_mmap iter = vbuffer_mmap_init;
	uint8 *data;
	size_t len;

	while ((data = vbuffer_mmap(buf, &len, false, &iter, NULL))) {
		if (len > 0) {
			inet_checksum_partial(csum, data, len);
		}
	}
}

int16 inet_checksum_vbuffer(struct vbuffer_sub *buf)
{
	struct checksum_partial csum = checksum_partial_init;
	inet_checksum_vbuffer_partial(&csum, buf);
	return inet_checksum_reduce(&csum);
}

bool ipv4_verify_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip, false);
	return inet_checksum((uint8 *)ipv4_header(ip, false), ipv4_get_hdr_len(ip)) == 0;
}

void ipv4_compute_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	struct ipv4_header *header = ipv4_header(ip, true);
	if (header) {
		header->checksum = 0;
		header->checksum = inet_checksum((uint8 *)header, ipv4_get_hdr_len(ip));
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
