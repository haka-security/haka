
#include <haka/module.h>


static int init(int argc, char *argv[])
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
	init:        init,
	cleanup:     cleanup
};
