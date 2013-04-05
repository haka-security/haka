
#include <haka/packet_module.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "app.h"


static filter_callback filter_function;
static void *filter_data;


int set_filter(filter_callback filter, void *data)
{
	filter_function = filter;
	filter_data = data;
}

int has_filter()
{
	return filter_function != NULL;
}
filter_result call_filter(lua_state *L, struct packet *pkt)

{
	return filter_function(L, filter_data, pkt);
}

