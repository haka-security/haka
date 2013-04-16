
#include "ipv4.h"

#include <stdlib.h>

#include <haka/log.h>


struct ipv4 *create(struct packet* packet)
{
	struct ipv4 *ip = NULL;

	ip = malloc(sizeof(struct ipv4));
	if (!ip) {
		return NULL;
	}

	ip->packet = packet;
	ip->header = (struct ipv4_header*)(packet_data(packet));

	return ip;
}

void release(struct ipv4 *ip)
{
	free(ip);
}

bool ipv4_verify_checksum(const struct ipv4 *ip)
{
	//TODO
	return true;
}

void ipv4_compute_checksum(struct ipv4 *ip)
{
	//TODO
}
