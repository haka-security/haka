
#ifndef _HAKA_PACKET_H
#define _HAKA_PACKET_H

#include <stddef.h>
#include <haka/types.h>
#include <haka/thread.h>
#include <haka/lua/object.h>


/* Opaque packet structure. */
struct packet {
	struct lua_object lua_object;
	atomic_t          ref;
};

struct packet_module_state;

enum packet_status {
	STATUS_NORMAL,
	STATUS_FORGED,
	STATUS_SENT,
};

bool               packet_init(struct packet_module_state *state);
void               packet_addref(struct packet *pkt);
bool               packet_release(struct packet *pkt);
struct packet     *packet_new(size_t size);
size_t             packet_length(struct packet *pkt);
const uint8       *packet_data(struct packet *pkt);
const char        *packet_dissector(struct packet *pkt);
uint8             *packet_data_modifiable(struct packet *pkt);
int                packet_resize(struct packet *pkt, size_t size);
void               packet_drop(struct packet *pkt);
void               packet_accept(struct packet *pkt);
bool               packet_send(struct packet *pkt);
int                packet_receive(struct packet **pkt);
size_t             packet_mtu(struct packet *pkt);
enum packet_status packet_state(struct packet *pkt);

enum packet_mode {
	MODE_NORMAL,
	MODE_PASSTHROUGH,
};

enum packet_mode   packet_mode();

#endif /* _HAKA_PACKET_H */

