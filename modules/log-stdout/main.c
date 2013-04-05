
#include "config.h"

#include <haka/log_module.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#ifdef USE_COLORS
	#define CLEAR_COLOR    L"\033[0m"

	static const char *level_color[LOG_LEVEL_LAST] = {
		"\033[1;31m", // LOG_FATAL
		"\033[1;31m", // LOG_ERROR
		"\033[1;33m", // LOG_WARNING
		"\033[0m", // LOG_INFO
		"\033[0m", // LOG_DEBUG
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
	const int module_size = wcslen(module);
	if (wcslen(module) > _max_module_size) {
		_max_module_size = module_size;
	}

#ifdef USE_COLORS
	fwprintf(stdout, L"%s%-5s: %-*ls: %ls\n" CLEAR_COLOR,
	         level_color[lvl], level_to_str(lvl), _max_module_size, module, message);
#else
	fwprintf(stdout, L"%-5s: %ls: %ls\n", level_to_str(lvl), module, message);
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

