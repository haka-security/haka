#include "haka/tcp.h"

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include <haka/log.h>
#include <haka/error.h>


struct tcp *tcp_dissect(struct ipv4 *packet)
{
	struct tcp *tcp = NULL;

	if (tcp_get_length(packet) < sizeof(struct tcp_header)) {
		return NULL;
	}

	/* Not a TCP packet */
	if (ipv4_get_proto(packet) != 6) {
		return NULL;
	}

	tcp = malloc(sizeof(struct tcp));
	if (!tcp) {
		return NULL;
	}
	tcp->packet = packet;
	tcp->header = (struct tcp_header*)(ipv4_get_payload(packet));
	tcp->modified = false;
	tcp->invalid_checksum = false;

	return tcp;
}

void tcp_forge(struct tcp *tcp)
{
	if (tcp->invalid_checksum)
		tcp_compute_checksum(tcp);
}

void tcp_release(struct tcp *tcp)
{
	free(tcp);
}


void tcp_pre_modify(struct tcp *tcp)
{
	if (!tcp->modified) {
		tcp->header = (struct tcp_header *)(packet_make_modifiable(tcp->packet->packet) + ipv4_get_hdr_len(tcp->packet) * 4);
	}
    tcp->modified = true;
}

void tcp_pre_modify_header(struct tcp *tcp)
{
    tcp_pre_modify(tcp);
    tcp->invalid_checksum = true;
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


int16 tcp_checksum(const struct tcp *tcp)
{
	struct tcp_pseudo_header *ptr = malloc(sizeof(struct tcp_pseudo_header));

	if (!ptr) {
		return 0;
	}

	/* fill tcp pseudo header */
	ptr->src = tcp->packet->header->src;
	ptr->dst = tcp->packet->header->dst;
	ptr->reserved = 0;
	ptr->proto = tcp->packet->header->proto;
	ptr->len = htons(tcp_get_length(tcp->packet));

	/* compute checksum */
	long sum;
	uint16 sum1, sum2;

	sum1 = ~inet_checksum((uint16 *)ptr, sizeof(struct tcp_pseudo_header));
    sum2 = ~inet_checksum((uint16 *)tcp->header, tcp_get_length(tcp->packet));

    sum = sum1 + sum2;

	while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

	sum = ~sum;

	free(ptr);

    return sum;
}


bool tcp_verify_checksum(const struct tcp *tcp)
{
    return tcp_checksum(tcp)  == 0;
}

void tcp_compute_checksum(struct tcp *tcp)
{
	   
	tcp_pre_modify_header(tcp);
    tcp->header->checksum = 0;
    tcp->header->checksum = tcp_checksum(tcp);
    tcp->invalid_checksum = false;
}

uint16 tcp_get_length(struct ipv4 *packet)
{
    uint8 hdr_len = ipv4_get_hdr_len(packet) * 4; //remove mult
    uint16 total_len = ipv4_get_len(packet);
    return total_len - hdr_len;
}

