
#include "haka/ipv4.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <haka/log.h>
#include <haka/error.h>


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
		error(L"invalid packet size");
		return NULL;
	}

	ip = malloc(sizeof(struct ipv4));
	if (!ip) {
		error(L"memory error");
		return NULL;
	}

	ip->packet = packet;
	ip->modified = false;
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

	ip->payload = vbuffer_extract(payload, hdrlen.hdr_len << IPV4_HDR_LEN_OFFSET, ALL, false);
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
	ip->modified = true;
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

		vbuffer_insert(payload, 0, header_buffer, true);
	}

	ip->payload = vbuffer_extract(payload, hdrlen, ALL, false);
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

		vbuffer_insert(ip->packet->payload, ipv4_get_hdr_len(ip), ip->payload, false);

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
	assert(len >= sizeof(struct ipv4_header));
	assert(header);
	return header;
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

bool ipv4_pre_modify(struct ipv4 *ip)
{
	if (!ip->modified) {
		if (!packet_is_modifiable(ip->packet)) {
			assert(check_error());
			return false;
		}

		ip->modified = true;
	}
	return true;
}

bool ipv4_pre_modify_header(struct ipv4 *ip)
{
	const bool ret = ipv4_pre_modify(ip);
	if (!ret) return ret;

	ip->invalid_checksum = true;
	return true;
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

bool ipv4_verify_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip, false);
	return inet_checksum((uint16 *)ipv4_header(ip, false), ipv4_get_hdr_len(ip)) == 0;
}

void ipv4_compute_checksum(struct ipv4 *ip)
{
	IPV4_CHECK(ip);
	if (ipv4_pre_modify_header(ip)) {
		struct ipv4_header *header = ipv4_header(ip, true);

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
