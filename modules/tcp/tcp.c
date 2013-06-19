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

struct tcp *tcp_dissect(struct ipv4 *packet)
{
	struct tcp *tcp = NULL;

	/* Not a TCP packet */
	if (ipv4_get_proto(packet) != TCP_PROTO) {
		error(L"Not a tcp packet");
		return NULL;
	}

	if (ipv4_get_payload_length(packet) < sizeof(struct tcp_header)) {
		error(L"TCP header length should have a minimum size of  %d", sizeof(struct tcp_header));
		return NULL;
	}

	tcp = malloc(sizeof(struct tcp));
	if (!tcp) {
		error(L"Failed to allocate memory");
		return NULL;
	}

	lua_object_init(&tcp->lua_object);
	tcp->packet = packet;
	tcp->header = (struct tcp_header*)(ipv4_get_payload(packet));
	tcp->modified = false;
	tcp->invalid_checksum = false;

	return tcp;
}

struct ipv4 *tcp_forge(struct tcp *tcp)
{
	struct ipv4 *packet = tcp->packet;
	if (packet) {
		if (tcp->invalid_checksum || packet->invalid_checksum)
			tcp_compute_checksum(tcp);

		tcp->packet = NULL;
		tcp->header = NULL;
		return packet;
	}
	else {
		return NULL;
	}
}

static void tcp_flush(struct tcp *tcp)
{
	struct ipv4 *packet;
	while ((packet = tcp_forge(tcp))) {
		ipv4_release(packet);
	}
}

void tcp_release(struct tcp *tcp)
{
	lua_object_release(tcp, &tcp->lua_object);
	tcp_flush(tcp);
	free(tcp);
}

void tcp_pre_modify(struct tcp *tcp)
{
	if (!tcp->modified) {
		tcp->header = (struct tcp_header *)(ipv4_get_payload_modifiable(tcp->packet));
	}
	tcp->modified = true;
	tcp->invalid_checksum = true;
}

int16 tcp_checksum(const struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);

	struct tcp_pseudo_header tcp_pseudo_h;

	/* fill tcp pseudo header */
	tcp_pseudo_h.src = tcp->packet->header->src;
	tcp_pseudo_h.dst = tcp->packet->header->dst;
	tcp_pseudo_h.reserved = 0;
	tcp_pseudo_h.proto = tcp->packet->header->proto;
	tcp_pseudo_h.len = SWAP_TO_BE(uint16, ipv4_get_payload_length(tcp->packet));

	/* compute checksum */
	long sum;
	uint16 sum1, sum2;

	sum1 = ~inet_checksum((uint16 *)&tcp_pseudo_h, sizeof(struct tcp_pseudo_header));
	sum2 = ~inet_checksum((uint16 *)tcp->header, ipv4_get_payload_length(tcp->packet));

	sum = sum1 + sum2;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	sum = ~sum;

	return sum;
}


bool tcp_verify_checksum(const struct tcp *tcp)
{
	TCP_CHECK(tcp, false);
	return tcp_checksum(tcp) == 0;
}

void tcp_compute_checksum(struct tcp *tcp)
{
	TCP_CHECK(tcp);
	tcp_pre_modify(tcp);
	tcp->header->checksum = 0;
	tcp->header->checksum = tcp_checksum(tcp);
	tcp->invalid_checksum = false;
}

const uint8 *tcp_get_payload(const struct tcp *tcp)
{
	TCP_CHECK(tcp, NULL);
	return ((const uint8 *)tcp->header) + tcp_get_hdr_len(tcp);
}

uint8 *tcp_get_payload_modifiable(struct tcp *tcp)
{
	TCP_CHECK(tcp, NULL);
	tcp_pre_modify(tcp);
	return (uint8 *)tcp_get_payload(tcp);
}

size_t tcp_get_payload_length(const struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return ipv4_get_payload_length(tcp->packet) - tcp_get_hdr_len(tcp);
}

uint8 *tcp_resize_payload(struct tcp *tcp, size_t size)
{
	TCP_CHECK(tcp, NULL);

	tcp->header = (struct tcp_header *)(ipv4_resize_payload(tcp->packet, size + tcp_get_hdr_len(tcp)));
	tcp->modified = true;
	tcp->invalid_checksum = true;
	return (uint8 *)tcp_get_payload(tcp);
}

void tcp_action_drop(struct tcp *tcp)
{
	TCP_CHECK(tcp);
	ipv4_action_drop(tcp->packet);
}

bool tcp_valid(struct tcp *tcp)
{
	TCP_CHECK(tcp, false);
	return ipv4_valid(tcp->packet);
}
