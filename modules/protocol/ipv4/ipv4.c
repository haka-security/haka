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
#include <haka/container/hash.h>


struct ipv4_frag_key {
	ipv4addr src;
	ipv4addr dst;
	uint32   id;
};

struct ipv4_frag_elem {
	hash_head_t       hh;
	struct list2      list;
};

struct ipv4_frag_table {
	mutex_t                 mutex;
	struct ipv4_frag_elem  *head;
};

static const size_t hash_keysize = sizeof(struct ipv4_frag_key);

static struct ipv4_frag_table *ipv4_frag_global;

static struct ipv4_frag_table *ipv4_frag_table_new()
{
	struct ipv4_frag_table *table = malloc(sizeof(struct ipv4_frag_table));
	if (!table) {
		error(L"memory error");
		return NULL;
	}

	/* Init a recursive mutex to avoid dead-lock on signal handling when
	 * running in single thread mode.
	 */
	UNUSED bool ret = mutex_init(&table->mutex, true);
	assert(ret);

	table->head = NULL;
	return table;
}

static void ipv4_frag_table_release(struct ipv4_frag_table *table)
{
	struct ipv4_frag_elem *elem, *tmp;

	HASH_ITER(hh, table->head, elem, tmp) {
		list2_iter iter, end = list2_end(&elem->list);
		for (iter = list2_begin(&elem->list);
		     iter != end; ) {
			struct ipv4 *cur = list2_get(iter, struct ipv4, frag_list);
			iter = list2_erase(iter);
			ipv4_release(cur);
		}

		HASH_DEL(table->head, elem);
		free(elem);
	}

	mutex_destroy(&table->mutex);
	free(table);
}

static bool ipv4_frag_insert(struct ipv4_frag_elem *elem, struct ipv4 *pkt)
{
	list2_iter iter = list2_begin(&elem->list);
	const list2_iter end = list2_end(&elem->list);
	size_t last_offset = 0;
	bool complete = true, added = false;

	if (list2_empty(&elem->list)) {
		list2_insert(end, &pkt->frag_list);
		return false;
	}

	for (; iter != end; iter = list2_next(iter)) {
		struct ipv4 *cur = list2_get(iter, struct ipv4, frag_list);

		size_t curoffset = ipv4_get_frag_offset(cur);
		ssize_t curlen;

		assert(ipv4_get_src(cur) == ipv4_get_src(pkt));
		assert(ipv4_get_dst(cur) == ipv4_get_dst(pkt));
		assert(ipv4_get_id(cur) == ipv4_get_id(pkt));

		if (!added && curoffset >= ipv4_get_frag_offset(pkt)) {
			if (!ipv4_get_flags_mf(cur)) {
				assert(list2_next(iter) == end);
				/* an alert will be raised bellow */
				break;
			}

			iter = list2_insert(iter, &pkt->frag_list);

			if (!complete) return false;

			added = true;
			iter = list2_prev(iter);
			continue;
		}

		curlen = ipv4_get_len(cur) - ipv4_get_hdr_len(cur);
		assert(curlen >= 0);

		if (curoffset > last_offset) {
			if (added) return false;

			complete = false;
		}

		last_offset = curoffset + curlen;
	}

	if (!added) {
		/* insert the packet at the end */
		struct ipv4 *cur = list2_get(list2_prev(end), struct ipv4, frag_list);
		if (!ipv4_get_flags_mf(cur)) {
			/* invalid packet */
			ALERT(invalid_packet, 1, 1)
				description: L"invalid ipv4 fragment",
				severity: HAKA_ALERT_LOW,
			ENDALERT

			alert(&invalid_packet);
			return false;
		}

		list2_insert(end, &pkt->frag_list);

		if (!complete) return false;
	}

	/* check for last mf flag */
	iter = list2_prev(end);
	{
		struct ipv4 *cur = list2_get(iter, struct ipv4, frag_list);
		if (ipv4_get_flags_mf(cur)) return false;
	}

	return true;
}

static struct ipv4_frag_elem *ipv4_frag_table_insert(struct ipv4_frag_table *table, struct ipv4 *pkt)
{
	struct ipv4_frag_key key;
	struct ipv4_header *header = ipv4_header(pkt, false);
	struct ipv4_frag_elem *ptr;
	bool ret = false;

	assert(ipv4_get_flags_mf(pkt) || ipv4_get_frag_offset(pkt) > 0);
	assert(header);

	key.src = header->src;
	key.dst = header->dst;
	key.id = header->id;

	mutex_lock(&table->mutex);

	HASH_FIND(hh, table->head, &key, hash_keysize, ptr);
	if (ptr) {
		ret = ipv4_frag_insert(ptr, pkt);
	}
	else {
		ptr = malloc(sizeof(struct ipv4_frag_elem));
		if (!ptr) {
			error(L"memory error");
			mutex_unlock(&table->mutex);
			return false;
		}

		list2_init(&ptr->list);
		list2_insert(list2_end(&ptr->list), &pkt->frag_list);

		HASH_ADD_KEYPTR(hh, table->head, &key, hash_keysize, ptr);
	}

	if (!ret) ptr = NULL;

	mutex_unlock(&table->mutex);

	return ptr;
}

INIT void ipv4_init()
{
	ipv4_frag_global = ipv4_frag_table_new();
	if (!ipv4_frag_global) {
		error(L"memory error");
	}
}

FINI void ipv4_final()
{
	ipv4_frag_table_release(ipv4_frag_global);
	ipv4_frag_global = NULL;
}

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

	if (!vbuffer_select(&header, &ip->packet_payload, &ip->select)) {
		assert(check_error());
		return false;
	}

	ip->payload = &ip->packet_payload;

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
		ALERT(invalid_packet, 1, 1)
			description: L"corrupted ip packet, size is too small",
			severity: HAKA_ALERT_LOW,
		ENDALERT

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
	ip->reassembled = false;
	list2_elem_init(&ip->frag_list);

	/* extract ip header len */
	vbuffer_begin(payload, &hdrleniter);
	*(uint8 *)&hdrlen = vbuffer_iterator_getbyte(&hdrleniter);

	header_len = hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET;
	if (header_len < sizeof(struct ipv4_header)) {
		ALERT(invalid_packet, 1, 1)
			description: L"corrupted ip packet",
			severity: HAKA_ALERT_LOW,
		ENDALERT

		alert(&invalid_packet);

		packet_drop(packet);
		packet_release(packet);
		free(ip);
		return NULL;
	}

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

struct ipv4 *ipv4_reassemble(struct ipv4 *ip)
{
	if (ipv4_get_flags_mf(ip) || ipv4_get_frag_offset(ip) > 0) {
		struct ipv4_frag_elem *elem = ipv4_frag_table_insert(ipv4_frag_global, ip);
		struct ipv4 *first;
		list2_iter iter, end;
		size_t offset = 0;

		if (!elem) return NULL;

		first = list2_first(&elem->list, struct ipv4, frag_list);
		assert(first);
		assert(!first->reassembled);

		first->reassembled = true;
		first->reassembled_offset = 0;
		vbuffer_stream_init(&first->reassembled_payload, NULL);

		end = list2_end(&elem->list);

		/* build ipv4 reassembled payload */
		for (iter = list2_begin(&elem->list); iter != end; ) {
			struct ipv4 *cur = list2_get(iter, struct ipv4, frag_list);
			const ssize_t curoff = ipv4_get_frag_offset(cur);
			const size_t curlen = vbuffer_size(&cur->packet_payload);

			assert(curoff <= offset);

			if (curoff != offset) {
				/* overlaps */
				if (curoff + curlen <= offset) {
					/* No data usefull in this packet */
					ipv4_action_drop(cur);
					iter = list2_erase(iter);
					ipv4_release(cur);
					continue;
				}
				else {
					/* Only a sub part of the data should be used, erase
					 * the rest */
					struct vbuffer_sub erased;
					vbuffer_sub_create(&erased, &cur->packet_payload, 0, offset - curoff);
					vbuffer_erase(&erased);
					vbuffer_sub_clear(&erased);
				}
			}

			vbuffer_stream_push(&first->reassembled_payload, &cur->packet_payload, cur, NULL);

			offset += curlen;
			iter = list2_erase(iter);
		}

		vbuffer_stream_finish(&first->reassembled_payload);

		first->payload = vbuffer_stream_data(&first->reassembled_payload);

		HASH_DEL(ipv4_frag_global->head, elem);
		free(elem);

		return first;
	}
	else {
		return ip;
	}
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
	ip->reassembled = false;
	list2_elem_init(&ip->frag_list);

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

static struct packet *ipv4_forge_one(struct ipv4 *ip, struct vbuffer *payload, size_t frag_offset, bool more)
{
	struct packet *packet = ip->packet;
	if (packet) {
		const size_t len = ipv4_get_hdr_len(ip) + vbuffer_size(payload);

		if (len != ipv4_get_len(ip)) ipv4_set_len(ip, len);

		if (frag_offset != (size_t)-1) {
			if (frag_offset != ipv4_get_frag_offset(ip)) ipv4_set_frag_offset(ip, frag_offset);
			if (more != ipv4_get_flags_mf(ip)) ipv4_set_flags_mf(ip, more);
		}

		if (ip->invalid_checksum) {
			ipv4_compute_checksum(ip);
		}

		vbuffer_restore(&ip->select, payload, false);

		ip->packet = NULL;
		return packet;
	}
	else {
		return NULL;
	}
}

struct packet *ipv4_forge(struct ipv4 *ip)
{
	if (!ip->reassembled) {
		return ipv4_forge_one(ip, ip->payload, -1, false);
	}
	else {
		struct vbuffer buffer;
		struct ipv4 *pkt = NULL;
		size_t offset = ip->reassembled_offset;
		struct packet *retpkt;
		UNUSED bool ret;
		bool more;

		while (!pkt) {
			ret = vbuffer_stream_pop(&ip->reassembled_payload, &buffer, (void **)&pkt);
			if (!ret) return NULL;

			if (vbuffer_isempty(&buffer)) {
				ipv4_action_drop(pkt);
				ipv4_release(pkt);
				pkt = NULL;
			}
		}

		more = vbuffer_check_size(vbuffer_stream_data(&ip->reassembled_payload), 1, NULL);

		ip->reassembled_offset += vbuffer_size(&buffer);
		retpkt = ipv4_forge_one(pkt, &buffer, offset, more);
		vbuffer_clear(&buffer);

		assert(retpkt);
		return retpkt;
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
		vbuffer_clear(&ip->packet_payload);
		ip->packet = NULL;
		ip->payload = NULL;
	}
}

void ipv4_release(struct ipv4 *ip)
{
	lua_object_release(ip, &ip->lua_object);
	ipv4_flush(ip);

	if (ip->reassembled) {
		vbuffer_stream_clear(&ip->reassembled_payload);
	}
	else {
		vbuffer_release(&ip->packet_payload);
	}

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
	return vbuffer_size(ip->payload);
}

void ipv4_action_drop(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	ipv4_flush(ip);
}
