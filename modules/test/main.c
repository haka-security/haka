
#include <haka/module.h>

#include <stdio.h>


static int init(int argc, char *argv[])
{
	return 1;
}

static void cleanup()
{
}


struct module HAKA_MODULE = {
	type:    UNKNOWN,
	init:    init,
	cleanup: cleanup
};

