
#include <haka/packet_module.h>

#include <stdio.h>


static int init(int argc, char *argv[])
{
	return 0;
}

static void cleanup()
{
}

static void *packet_receive()
{
	return NULL;
}

static void packet_verdict(void *pkt, filter_result result)
{
}


struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        "Test",
		description: "Template packet module",
		author:      "Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	receive:         packet_receive,
	verdict:         packet_verdict
};

