
#ifndef _HAKA_PACKET_MODULE_H
#define _HAKA_PACKET_MODULE_H

#include <haka/module.h>
#include <haka/filter_module.h>


struct packet_module {
	struct module    module;

	void*          (*receive)();
	void           (*verdict)(void *pkt, filter_result result);
};

#endif /* _HAKA_PACKET_MODULE_H */

