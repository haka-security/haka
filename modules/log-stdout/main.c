
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

static int log_message(log_level lvl, const char *module, const char *message)
{
    fprintf(stderr, "%s: %s: %s\n", level_to_str(lvl), module, message);
    return 0;
}


struct log_module HAKA_MODULE = {
	module: {
		type:        MODULE_LOG,
		name:        "Stdout logger",
		description: "Basic logger to stdout",
		author:      "Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	message:         log_message
};

