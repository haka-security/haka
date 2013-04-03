
#ifndef _HAKA_LOG_MODULE_H
#define _HAKA_LOG_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/log.h>


struct log_module {
	struct module    module;

	int            (*message)(log_level lvl, const wchar_t *module, const wchar_t *message);
};

#endif /* _HAKA_LOG_MODULE_H */

