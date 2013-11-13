
#ifndef _HAKA_ALERT_MODULE_H
#define _HAKA_ALERT_MODULE_H

#include <wchar.h>
#include <haka/module.h>
#include <haka/alert.h>


struct alerter_module {
	struct alerter        alerter;
	struct alert_module  *module;
};

struct alert_module {
	struct module    module;

	struct alerter_module *(*init_alerter)(struct parameters *args);
	void                   (*cleanup_alerter)(struct alerter_module *alerter);
};

struct alerter *alert_module_alerter(struct module *module, struct parameters *args);

#endif /* _HAKA_ALERT_MODULE_H */
