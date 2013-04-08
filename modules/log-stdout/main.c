
#include "config.h"

#include <haka/log_module.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#ifdef USE_COLORS
	#define CLEAR_COLOR    "\033[0m"

	static const char *level_color[LOG_LEVEL_LAST] = {
		"\033[1;31m", // LOG_FATAL
		"\033[1;31m", // LOG_ERROR
		"\033[1;33m", // LOG_WARNING
		CLEAR_COLOR,  // LOG_INFO
		CLEAR_COLOR,  // LOG_DEBUG
	};
#endif /* USE_COLORS */

static int _max_module_size = 0;

static int init(int argc, char *argv[])
{
	return 0;
}

static void cleanup()
{
}

static int log_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	const char *level_str = level_to_str(lvl);
	const int level_size = strlen(level_str);
	const int module_size = wcslen(module);

	if (wcslen(module) > _max_module_size) {
		_max_module_size = module_size;
	}

#ifdef USE_COLORS
	fwprintf(stdout, L"%s%s:%*s %ls:%*s %ls%s\n", level_color[lvl], level_str,
			level_size-5, "", module, _max_module_size-module_size, "", message,
			CLEAR_COLOR);
#else
	fwprintf(stdout, L"%s:%*s %ls:%*s %ls\n", level_str, level_size-5, "",
			module, _max_module_size-module_size, "", message);
#endif /* USES_COLORS */

	return 0;
}


struct log_module HAKA_MODULE = {
	module: {
		type:        MODULE_LOG,
		name:        L"Stdout logger",
		description: L"Basic logger to stdout",
		author:      L"Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	message:         log_message
};

