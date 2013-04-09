
#include <haka/packet_module.h>
#include <haka/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pcap.h>
#include <string.h>


struct packet {
	struct pcap_pkthdr header;
	const char *data;
};

static pcap_t *pd;

static int init(int argc, char *argv[])
{
	if(argc == 2) {
	    char errbuf[PCAP_ERRBUF_SIZE];

		/* get a pcap descriptor from device */
		if(strcmp(argv[0], "-i") == 0) {
			pd = pcap_open_live(argv[1], 1518, 1, 0, errbuf);
			if(!pd) {
				messagef(LOG_ERROR, L"pcap", L"%s", errbuf);
				return 1;
			}
		}	
		else if(strcmp(argv[0], "-f") == 0) {
		/* get a pcap descriptor from a pcap file */
            pd = pcap_open_offline(argv[1], errbuf);
            if(!pd) {
                messagef(LOG_ERROR, L"pcap", L"%s", errbuf);
				return 1;
			}
		}
		else
			messagef(LOG_ERROR, L"pcap", L"Unkown options");
	}
	else {
		messagef(LOG_ERROR, L"pcap", L"Specify a device (-i) or a pcap filename (-f)");
		return 1;
	}
	return 0;
}

static void cleanup()
{
	pcap_close(pd);
}

static int packet_receive(struct packet **pkt)
{
	struct packet *packet = NULL;

	packet = malloc(sizeof(struct packet));
	if (!packet) {
		return ENOMEM;
	}
	
	/* read packet */
	if(!(packet->data = pcap_next(pd, &packet->header)))
	{
		free(packet);
		return 1;
	}

	*pkt = packet;
	return 0;
}

static void packet_verdict(struct packet *pkt, filter_result result)
{
	free(pkt);
}

static size_t packet_get_length(struct packet *pkt)
{
	return pkt->header.caplen;
}

static const char *packet_get_data(struct packet *pkt)
{
	return pkt->data;
}


struct packet_module HAKA_MODULE = {
	module: {
		type:        MODULE_PACKET,
		name:        L"Pcap Module",
		description: L"Pcap packet module",
		author:      L"Arkoon Network Security",
		init:        init,
		cleanup:     cleanup
	},
	receive:         packet_receive,
	verdict:         packet_verdict,
	get_length:      packet_get_length,
	get_data:        packet_get_data
};

