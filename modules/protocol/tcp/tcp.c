#include "haka/tcp.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <haka/log.h>
#include <haka/error.h>

struct tcp_pseudo_header {
	ipv4addr       src;
	ipv4addr       dst;
	uint8          reserved;
	uint8          proto;
	uint16         len;
};

static bool tcp_flatten_header(struct vbuffer *payload, size_t hdrlen)
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

struct tcp *tcp_dissect(struct ipv4 *packet)
{
	struct tcp *tcp = NULL;
	struct {
#ifdef HAKA_LITTLEENDIAN
	uint8    res:4, hdr_len:4;
#else
	uint8    hdr_len:4, res:4;
#endif
	} hdrlen;

	/* Not a TCP packet */
	if (ipv4_get_proto(packet) != TCP_PROTO) {
		error(L"Not a tcp packet");
		return NULL;
	}

	if (!vbuffer_checksize(packet->payload, sizeof(struct tcp_header))) {
		error(L"TCP header length should have a minimum size of %d", sizeof(struct tcp_header));
		return NULL;
	}

	tcp = malloc(sizeof(struct tcp));
	if (!tcp) {
		error(L"Failed to allocate memory");
		return NULL;
	}

	tcp->packet = packet;
	tcp->modified = false;
	tcp->invalid_checksum = false;

	/* extract header len (at offset 12, see struct tcp_header) */
	*(uint8 *)&hdrlen = vbuffer_getbyte(packet->payload, 12);

	if (!tcp_flatten_header(packet->payload, hdrlen.hdr_len << TCP_HDR_LEN)) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	tcp->payload = vbuffer_extract(packet->payload, hdrlen.hdr_len << TCP_HDR_LEN, ALL, false);
	if (!tcp->payload) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	lua_object_init(&tcp->lua_object);

	return tcp;
}

static bool tcp_add_header(struct vbuffer *payload)
{
	const size_t hdrlen = sizeof(struct tcp_header);
	size_t len;
	uint8 *ptr;
	struct vbuffer *header_buffer = vbuffer_create_new(hdrlen);
	if (!header_buffer) {
		assert(check_error());
		return false;
	}

	ptr = vbuffer_mmap(header_buffer, NULL, &len, true);
	assert(ptr);
	memset(ptr, 0, len);

	vbuffer_insert(payload, 0, header_buffer, true);
	return true;
}

struct tcp *tcp_create(struct ipv4 *packet)
{
	struct vbuffer *payload;
	size_t hdrlen;

	struct tcp *tcp = malloc(sizeof(struct tcp));
	if (!tcp) {
		error(L"Failed to allocate memory");
		return NULL;
	}

	payload = packet->payload;

	hdrlen = sizeof(struct tcp_header);
	if (!tcp_add_header(payload)) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	tcp->payload = vbuffer_extract(payload, hdrlen, ALL, false);
	if (!tcp->payload) {
		assert(check_error());
		free(tcp);
		return NULL;
	}

	tcp->packet = packet;
	tcp->modified = true;
	tcp->invalid_checksum = true;
	lua_object_init(&tcp->lua_object);

	ipv4_set_proto(packet, TCP_PROTO);
	tcp_set_checksum(tcp, 0);
	tcp_set_hdr_len(tcp, sizeof(struct tcp_header));

	return tcp;
}

static void tcp_recompute_checksum(struct tcp *tcp)
{
	if (tcp->invalid_checksum || tcp->packet->invalid_checksum ||
		vbuffer_ismodified(tcp->payload)) {
		tcp_compute_checksum(tcp);
	}
}

struct ipv4 *_tcp_forge(struct tcp *tcp, bool split)
{
	struct ipv4 *packet = tcp->packet;
	if (packet) {
		const size_t mtu = packet_mtu(packet->packet) - ipv4_get_hdr_len(packet) - tcp_get_hdr_len(tcp);

		if (split && packet_mode() != MODE_PASSTHROUGH && vbuffer_checksize(tcp->payload, mtu+1)) {
			struct vbuffer *rem_payload;
			struct packet *rem_pkt;
			struct ipv4 *rem_ip;
			struct ipv4_header ipheader;
			struct tcp_header tcpheader;
			struct tcp_header *header;

			rem_payload = vbuffer_extract(tcp->payload, mtu, ALL, true);
			if (!rem_payload) {
				assert(check_error());
				return NULL;
			}

			ipheader = *ipv4_header(tcp->packet, false);
			tcpheader = *tcp_header(tcp, false);

			tcp_set_flags_fin(tcp, false);
			tcp_set_flags_psh(tcp, false);

			tcp_recompute_checksum(tcp);

			vbuffer_insert(tcp->packet->payload, tcp_get_hdr_len(tcp), tcp->payload, true);
			tcp->payload = NULL;

			/* 'packet' is ready to be sent, preapre the next tcp packet
			 * before returning */
			tcp->payload = rem_payload;

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

			vbuffer_insert(tcp->packet->payload, tcp_get_hdr_len(tcp), tcp->payload, false);
			tcp->payload = NULL;

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
	struct tcp_header *header;
	size_t len;
	header = (struct tcp_header *)vbuffer_mmap(tcp->packet->payload, NULL, &len, write);
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
	struct ipv4 *packet;
	while ((packet = _tcp_forge(tcp, false))) {
		ipv4_release(packet);
	}
}

void tcp_release(struct tcp *tcp)
{
	lua_object_release(tcp, &tcp->lua_object);
	tcp_flush(tcp);
	if (tcp->payload) {
		vbuffer_free(tcp->payload);
		tcp->payload = NULL;
	}
	free(tcp);
}

int16 tcp_checksum(struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);

	struct tcp_pseudo_header tcp_pseudo_h;
	void *iter = NULL;
	uint8 *data;
	size_t len;

	/* fill tcp pseudo header */
	struct ipv4_header *ipheader = ipv4_header(tcp->packet, false);
	tcp_pseudo_h.src = ipheader->src;
	tcp_pseudo_h.dst = ipheader->dst;
	tcp_pseudo_h.reserved = 0;
	tcp_pseudo_h.proto = ipheader->proto;
	tcp_pseudo_h.len = SWAP_TO_IPV4(uint16, ipv4_get_payload_length(tcp->packet) + tcp_get_payload_length(tcp));

	/* compute checksum */
	long sum;

	sum = (uint16)~inet_checksum((uint16 *)&tcp_pseudo_h, sizeof(struct tcp_pseudo_header));

	while ((data = vbuffer_mmap(tcp->packet->payload, &iter, &len, false))) {
		sum += (uint16)~inet_checksum((uint16 *)data, len);
	}

	while ((data = vbuffer_mmap(tcp->payload, &iter, &len, false))) {
		sum += (uint16)~inet_checksum((uint16 *)data, len);
	}

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	sum = ~sum;

	return sum;
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

const uint8 *tcp_get_payload(struct tcp *tcp)
{
	TCP_CHECK(tcp, NULL);
	vbuffer_flatten(tcp->payload);
	return vbuffer_mmap(tcp->payload, NULL, NULL, false);
}

uint8 *tcp_get_payload_modifiable(struct tcp *tcp)
{
	TCP_CHECK(tcp, NULL);
	vbuffer_flatten(tcp->payload);
	return vbuffer_mmap(tcp->payload, NULL, NULL, true);
}

size_t tcp_get_payload_length(struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return vbuffer_size(tcp->payload);
}

uint8 *tcp_resize_payload(struct tcp *tcp, size_t size)
{
	size_t cursize;
	TCP_CHECK(tcp, NULL);

	cursize = vbuffer_size(tcp->payload);
	if (size > cursize) {
		struct vbuffer *extra = vbuffer_create_new(size-cursize);
		if (!extra) {
			assert(check_error());
			return NULL;
		}

		vbuffer_insert(tcp->payload, cursize, extra, true);
	}
	else {
		vbuffer_erase(tcp->payload, size, ALL);
	}

	tcp->modified = true;
	tcp->invalid_checksum = true;
	return (uint8 *)tcp_get_payload(tcp);
}

void tcp_action_drop(struct tcp *tcp)
{
	TCP_CHECK(tcp);
	tcp_flush(tcp);
}
