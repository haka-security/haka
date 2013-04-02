
#include <haka/packet_module.h>
#include <haka/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


struct packet {
	size_t    length;
	char     *data;
};


static int init(int argc, char *argv[])
{
	return 0;
}

static void cleanup()
{
}

static int packet_receive(struct packet **pkt)
{
	struct packet *packet = NULL;

    sleep(1);

	packet = malloc(sizeof(struct packet));
	if (!packet) {
		return ENOMEM;
	}

	packet->length = 4;
	packet->data = "test";
	*pkt = packet;

	return 0;
}

static void packet_verdict(struct packet *pkt, filter_result result)
{
    messagef(LOG_DEBUG, "test", "verdict: %d", result);
	free(pkt);
}

static size_t packet_length(struct packet *pkt)
{
	return pkt->length;
}

static const char *packet_data(struct packet *pkt)
{
	return pkt->data;
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
	verdict:         packet_verdict,
	get_length:      packet_length,
	get_data:        packet_data
};

