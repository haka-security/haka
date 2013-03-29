
#ifndef _HAKA_FILTER_MODULE_H
#define _HAKA_FILTER_MODULE_H

#include <haka/module.h>


typedef enum {
	FILTER_ACCEPT,
	FILTER_DROP
} filter_result;

struct filter_module {
	struct module    module;

	filter_result  (*filter)(void *pkt);
};

#endif /* _HAKA_FILTER_MODULE_H */

