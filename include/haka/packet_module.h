
#ifndef _HAKA_PACKET_MODULE_H
#define _HAKA_PACKET_MODULE_H

#include <haka/module.h>
#include <stddef.h>


struct packet;

typedef enum {
	FILTER_ACCEPT,
	FILTER_DROP
} filter_result;

struct packet_module {
	struct module    module;

	int            (*receive)(struct packet **pkt);
	void           (*verdict)(struct packet *pkt, filter_result result);

	size_t         (*get_length)(struct packet *pkt);
	const char *   (*get_data)(struct packet *pkt);
};

#endif /* _HAKA_PACKET_MODULE_H */

