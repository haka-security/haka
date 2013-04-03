
#ifndef _HAKA_PACKET_H
#define _HAKA_PACKET_H

#include <stddef.h>


struct packet;

size_t      packet_length(struct packet *pkt);
const char *packet_data(struct packet *pkt);

#endif /* _HAKA_PACKET_H */

