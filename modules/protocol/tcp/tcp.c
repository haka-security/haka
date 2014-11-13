/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "haka/tcp.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <wchar.h>

#include <haka/log.h>
#include <haka/alert.h>
#include <haka/error.h>
#include <haka/string.h>


struct tcp_pseudo_header {
	ipv4addr       src;
	ipv4addr       dst;
	uint8          reserved;
	uint8          proto;
	uint16         len;
};

static void alert_invalid_packet(struct ipv4 *packet, char *desc)
{
	TOSTR(srcip, ipv4addr, ipv4_get_src(packet));
	TOSTR(dstip, ipv4addr, ipv4_get_dst(packet));
	ALERT(invalid_packet, 1, 1)
		description: desc,
		severity: HAKA_ALERT_LOW,
	ENDALERT

	ALERT_NODE(invalid_packet, sources, 0, HAKA_ALERT_NODE_ADDRESS, srcip);
	ALERT_NODE(invalid_packet, targets, 0, HAKA_ALERT_NODE_ADDRESS, dstip);

	alert(&invalid_packet);
}

static bool tcp_flatten_header(struct vbuffer *payload, size_t hdrlen)
{
	struct vbuffer_sub header_part;
	size_t len;
	const uint8 *ptr;

	vbuffer_sub_create(&header_part, payload, 0, hdrlen);

	/* Skip empty chunk to avoid issue if the buffer begin with
	 * a control node (which could be the case when receiving fragmented
	 * ip packets).
	 */
	vbuffer_iterator_skip_empty(&header_part.begin);

	ptr = vbuffer_sub_flatten(&header_part, &len);
	assert(len >= hdrlen);
	return ptr != NULL;
}

static bool tcp_extract_payload(struct tcp *tcp, size_t hdrlen, size_t size)
{
	struct vbuffer_sub header;

	vbuffer_sub_create(&header, tcp->packet->payload, hdrlen, ALL);

	if (!vbuffer_select(&header, &tcp->payload, &tcp->select)) {
		assert(check_error());
		free(tcp);
		return false;
	}

	return true;
}

struct tcp *tcp_dissect(struct ipv4 *packet)
{
	struct tcp *tcp = NULL;
	struct vbuffer_iterator hdrleniter;
	struct {
#ifdef HAKA_LITTLEENDIAN
	uint8    res:4, hdr_len:4;
#else
	uint8    hdr_len:4, res:4;
#endif
	} _hdrlen;
	size_t hdrlen;

	/* Not a TCP packet */
	if (ipv4_get_proto(packet) != TCP_PROTO) {
		error("Not a tcp packet");
		return NULL;
	}

	assert(packet->payload);

	if (!vbuffer_check_size(packet->payload, sizeof(struct tcp_header), NULL)) {
		alert_invalid_packet(packet, "corrupted tcp packet, size is too small");
		ipv4_action_drop(packet);
		return NULL;
	}

	tcp = malloc(sizeof(struct tcp));
	if (!tcp) {
		error("Failed to allocate memory");
		return NULL;
	}

	tcp->packet = packet;
	tcp->modified = false;
	tcp->invalid_checksum = false;

	/* extract header len (at offset 12, see struct tcp_header) */
	vbuffer_position(packet->payload, &hdrleniter, 12);
	*(uint8 *)&_hdrlen = vbuffer_iterator_getbyte(&hdrleniter);
	hdrlen = _hdrlen.hdr_len << TCP_HDR_LEN;

	if (hdrlen < sizeof(struct tcp_header) || !vbuffer_check_size(packet->payload, hdrlen, NULL)) {
		alert_invalid_packet(packet, "corrupted tcp packet, header length is too small");
		ipv4_action_drop(packet);
		free(tcp);
		return NULL;
	}

	if (!tcp_flatten_header(packet->payload, hdrlen)) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	if (!tcp_extract_payload(tcp, hdrlen, ALL)) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	tcp->lua_object = lua_object_init;
	tcp->lua_flags_object = lua_object_init;
	return tcp;
}

static bool tcp_add_header(struct vbuffer *payload)
{
	bool ret;
	const size_t hdrlen = sizeof(struct tcp_header);
	struct vbuffer header_buffer;
	struct vbuffer_iterator begin;
	if (!vbuffer_create_new(&header_buffer, hdrlen, true)) {
		assert(check_error());
		return false;
	}

	vbuffer_begin(payload, &begin);
	ret = vbuffer_iterator_insert(&begin, &header_buffer, NULL);
	vbuffer_release(&header_buffer);

	return ret;
}

struct tcp *tcp_create(struct ipv4 *packet)
{
	size_t hdrlen;

	struct tcp *tcp = malloc(sizeof(struct tcp));
	if (!tcp) {
		error("Failed to allocate memory");
		return NULL;
	}

	assert(packet->payload);

	hdrlen = sizeof(struct tcp_header);
	if (!tcp_add_header(packet->payload)) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	tcp->packet = packet;
	tcp->modified = true;
	tcp->invalid_checksum = true;

	if (!tcp_extract_payload(tcp, hdrlen, ALL)) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	tcp->lua_object = lua_object_init;
	tcp->lua_flags_object = lua_object_init;

	ipv4_set_proto(packet, TCP_PROTO);
	tcp_set_checksum(tcp, 0);
	tcp_set_hdr_len(tcp, sizeof(struct tcp_header));

	return tcp;
}

static void tcp_recompute_checksum(struct tcp *tcp)
{
	if (tcp->invalid_checksum || tcp->packet->invalid_checksum ||
		vbuffer_ismodified(&tcp->payload)) {
		tcp_compute_checksum(tcp);
	}
}

struct ipv4 *_tcp_forge(struct tcp *tcp, bool split)
{
	struct ipv4 *packet = tcp->packet;
	if (packet) {
		const size_t mtu = packet_mtu(packet->packet) - ipv4_get_hdr_len(packet) - tcp_get_hdr_len(tcp);

		if (split && packet_mode() != MODE_PASSTHROUGH && vbuffer_check_size(&tcp->payload, mtu+1, NULL)) {
			struct vbuffer rem_payload;
			struct packet *rem_pkt;
			struct ipv4 *rem_ip;
			struct ipv4_header ipheader;
			struct tcp_header tcpheader;
			struct tcp_header *header;

			{
				struct vbuffer_sub sub;
				vbuffer_sub_create(&sub, &tcp->payload, mtu, ALL);

				if (!vbuffer_extract(&sub, &rem_payload)) {
					assert(check_error());
					return NULL;
				}
			}

			ipheader = *ipv4_header(tcp->packet, false);
			tcpheader = *tcp_header(tcp, false);

			tcp_set_flags_fin(tcp, false);
			tcp_set_flags_psh(tcp, false);

			tcp_recompute_checksum(tcp);

			if (!vbuffer_iterator_isvalid(&tcp->select)) {
				struct vbuffer_iterator insert;
				vbuffer_position(tcp->packet->payload, &insert, tcp_get_hdr_len(tcp));
				vbuffer_iterator_insert(&insert, &tcp->payload, NULL);
			}
			else {
				vbuffer_restore(&tcp->select, &tcp->payload, false);
			}

			/* 'packet' is ready to be sent, prepare the next tcp packet
			 * before returning */
			vbuffer_swap(&tcp->payload, &rem_payload);
			vbuffer_release(&rem_payload);

			rem_pkt = packet_new(0);
			if (!rem_pkt) {
				assert(check_error());
				return NULL;
			}

			rem_ip = ipv4_create(rem_pkt);
			if (!rem_ip) {
				assert(check_error());
				packet_release(rem_pkt);
				return NULL;
			}

			if (!tcp_add_header(rem_ip->payload)) {
				assert(check_error());
				ipv4_release(rem_ip);
				return NULL;
			}

			*ipv4_header(rem_ip, true) = ipheader;
			ipv4_set_id(rem_ip, 0);

			tcp->packet = rem_ip;

			header = tcp_header(tcp, true);
			assert(header);
			*header = tcpheader;

			tcp_set_seq(tcp, SWAP_FROM_TCP(uint32, tcpheader.seq) + mtu);
			tcp_set_hdr_len(tcp, sizeof(struct tcp_header));

			return packet;
		}
		else {
			tcp_recompute_checksum(tcp);

			if (!vbuffer_iterator_isvalid(&tcp->select)) {
				struct vbuffer_iterator insert;
				vbuffer_position(tcp->packet->payload, &insert, tcp_get_hdr_len(tcp));
				vbuffer_iterator_insert(&insert, &tcp->payload, NULL);
			}
			else {
				vbuffer_restore(&tcp->select, &tcp->payload, false);
			}

			tcp->packet = NULL;
			return packet;
		}
	}
	else {
		return NULL;
	}
}

struct ipv4 *tcp_forge(struct tcp *tcp)
{
	return _tcp_forge(tcp, true);
}

struct tcp_header *tcp_header(struct tcp *tcp, bool write)
{
	struct vbuffer_iterator begin;
	struct tcp_header *header;
	size_t len;

	vbuffer_begin(tcp->packet->payload, &begin);

	header = (struct tcp_header *)vbuffer_iterator_mmap(&begin, ALL, &len, write);
	if (!header) {
		assert(write); /* should always work in read mode */
		assert(check_error());
		return NULL;
	}

	if (write) {
		tcp->invalid_checksum = true;
	}

	assert(len >= sizeof(struct tcp_header));
	return header;
}

static void tcp_flush(struct tcp *tcp)
{
	if (tcp->packet) {
		ipv4_release(tcp->packet);
		tcp->packet = NULL;
	}
}

void tcp_release(struct tcp *tcp)
{
	lua_object_release(&tcp->lua_object);
	lua_object_release(&tcp->lua_flags_object);
	tcp_flush(tcp);
	vbuffer_release(&tcp->payload);
	free(tcp);
}

int16 tcp_checksum(struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);

	struct tcp_pseudo_header tcp_pseudo_h;
	struct vbuffer_sub sub;
	struct checksum_partial csum = checksum_partial_init;

	/* fill tcp pseudo header */
	struct ipv4_header *ipheader = ipv4_header(tcp->packet, false);
	tcp_pseudo_h.src = ipheader->src;
	tcp_pseudo_h.dst = ipheader->dst;
	tcp_pseudo_h.reserved = 0;
	tcp_pseudo_h.proto = ipheader->proto;
	tcp_pseudo_h.len = SWAP_TO_IPV4(uint16, ipv4_get_payload_length(tcp->packet) + tcp_get_payload_length(tcp));

	/* compute checksum */
	inet_checksum_partial(&csum, (uint8 *)&tcp_pseudo_h, sizeof(struct tcp_pseudo_header));

	vbuffer_sub_create(&sub, tcp->packet->payload, 0, ALL);
	inet_checksum_vbuffer_partial(&csum, &sub);

	vbuffer_sub_create(&sub, &tcp->payload, 0, ALL);
	inet_checksum_vbuffer_partial(&csum, &sub);

	return inet_checksum_reduce(&csum);
}

bool tcp_verify_checksum(struct tcp *tcp)
{
	TCP_CHECK(tcp, false);
	return tcp_checksum(tcp) == 0;
}

void tcp_compute_checksum(struct tcp *tcp)
{
	TCP_CHECK(tcp);
	struct tcp_header *header = tcp_header(tcp, true);
	if (header) {
		header->checksum = 0;
		header->checksum = tcp_checksum(tcp);
		tcp->invalid_checksum = false;
	}
}

size_t tcp_get_payload_length(struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return vbuffer_size(&tcp->payload);
}

void tcp_action_drop(struct tcp *tcp)
{
	TCP_CHECK(tcp);
	tcp_flush(tcp);
}
