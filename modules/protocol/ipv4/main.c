
#include <haka/module.h>


static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        L"IPv4",
	description: L"IPv4 protocol",
	author:      L"Arkoon Network Security",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
