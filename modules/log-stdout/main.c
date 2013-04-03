
#include <haka/log_module.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


static int init(int argc, char *argv[])
{
	return 0;
}

static void cleanup()
{
}

static int log_message(log_level lvl, const wchar_t *module, const wchar_t *message)
{
	fwprintf(stderr, L"%s: %ls: %ls\n", level_to_str(lvl), module, message);
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

