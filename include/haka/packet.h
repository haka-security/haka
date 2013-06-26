
#ifndef _HAKA_PACKET_H
#define _HAKA_PACKET_H

#include <stddef.h>
#include <haka/types.h>
#include <haka/lua/object.h>


/* Opaque packet structure. */
struct packet {
	struct lua_object lua_object;
};

struct packet_module_state;

size_t       packet_length(struct packet *pkt);
const uint8 *packet_data(struct packet *pkt);
const char  *packet_dissector(struct packet *pkt);
uint8       *packet_data_modifiable(struct packet *pkt);
int          packet_resize(struct packet *pkt, size_t size);
void         packet_drop(struct packet *pkt);
void         packet_accept(struct packet *pkt);
int          packet_receive(struct packet_module_state *state, struct packet **pkt);

#endif /* _HAKA_PACKET_H */

