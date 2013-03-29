
#include <haka/filter_module.h>
#include <stdlib.h>

#include "state.h"


static struct lua_State *global_state;

static int init(int argc, char *argv[])
{
	global_state = init_state();
	if (!global_state) {
		return 1;
	}

	return 0;
}

static void cleanup()
{
	cleanup_state(global_state);
	global_state = NULL;
}

static filter_result packet_filter(void *pkt)
{
	return FILTER_ACCEPT;
}


struct filter_module HAKA_MODULE = {
	module: {
		type:        MODULE_FILTER,
		name:        "Lua",
		description: "Lua filter",
		author:      "Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	filter:          packet_filter
};

