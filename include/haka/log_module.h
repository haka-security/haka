
#ifndef _HAKA_LOG_MODULE_H
#define _HAKA_LOG_MODULE_H

#include <haka/module.h>
#include <haka/log.h>


struct log_module {
	struct module    module;

	int            (*message)(log_level lvl, const char *module, const char *message);
};

#endif /* _HAKA_LOG_MODULE_H */

