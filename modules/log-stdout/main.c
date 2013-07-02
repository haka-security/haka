
#include "config.h"

#include <haka/log_module.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#ifdef USE_COLORS
	#define CLEAR   "\e[0m"
	#define BOLD    "\e[1m"
	#define BLACK   "\e[30m"
	#define RED     "\e[31m"
	#define GREEN   "\e[32m"
	#define YELLOW  "\e[33m"
	#define BLUE    "\e[34m"
	#define MAGENTA "\e[35m"
	#define CYAN    "\e[36m"
	#define WHITE   "\e[37m"

	#define MODULE_COLOR   CYAN

	static const char *level_color[HAKA_LOG_LEVEL_LAST] = {
		BOLD RED,    // LOG_FATAL
		BOLD RED,    // LOG_ERROR
		BOLD YELLOW, // LOG_WARNING
		BOLD,        // LOG_INFO
		CLEAR,       // LOG_DEBUG
	};

	static const char *message_color[HAKA_LOG_LEVEL_LAST] = {
		CLEAR,       // LOG_FATAL
		CLEAR,       // LOG_ERROR
		CLEAR,       // LOG_WARNING
		CLEAR,       // LOG_INFO
		CLEAR,       // LOG_DEBUG
	};
#endif /* USE_COLORS */

static int _max_module_size = 0;
static mutex_t mutex;


static int init(int argc, char *argv[])
{
	if (!mutex_init(&mutex, true)) {
		return 1;
	}
	return 0;
}

static void cleanup()
{
	mutex_destroy(&mutex);
}

static int log_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	const char *level_str = level_to_str(lvl);
	const int level_size = strlen(level_str);
	const int module_size = wcslen(module);

	mutex_lock(&mutex);

	if (wcslen(module) > _max_module_size) {
		_max_module_size = module_size;
	}

#ifdef USE_COLORS
	fprintf(stdout, "%s%s%s%*s %s%ls:%s%*s %s%ls%s\n", level_color[lvl], level_str,
			CLEAR, level_size-5, "", MODULE_COLOR, module, CLEAR,
			_max_module_size-module_size, "", message_color[lvl], message, CLEAR);
#else
	fprintf(stdout, "%s%*s %ls:%*s %ls\n", level_str, level_size-5, "",
			module, _max_module_size-module_size, "", message);
#endif /* USES_COLORS */

	mutex_unlock(&mutex);

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
