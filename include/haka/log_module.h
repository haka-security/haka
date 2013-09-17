
#ifndef _HAKA_LOG_MODULE_H
#define _HAKA_LOG_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/log.h>


struct log_module_state;

struct log_module {
	struct module    module;

	struct log_module_state *(*init_state)(struct parameters *args);
	void           (*cleanup_state)(struct log_module_state *state);
	int            (*message)(struct log_module_state *state, log_level level, const wchar_t *module, const wchar_t *message);
};

#endif /* _HAKA_LOG_MODULE_H */

