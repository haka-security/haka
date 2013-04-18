
#include <haka/packet_module.h>
#include <haka/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pcap.h>
#include <string.h>

/* snapshot length - a value of 65535 is sufficient to get all
 * of the packet's data on most netwroks (from pcap man page)
 */
#define SNAPLEN 65535

struct packet {
	struct pcap_pkthdr header;
	u_char data[0];
};

static pcap_t *pd;
static pcap_dumper_t *pf;

static int init(int argc, char *argv[])
{
	if (argc == 2 || argc == 4) {
		char errbuf[PCAP_ERRBUF_SIZE];
		bzero(errbuf, PCAP_ERRBUF_SIZE);

		/* get a pcap descriptor from device */
		if (strcmp(argv[0], "-i") == 0) {
			pd = pcap_open_live(argv[1], SNAPLEN, 1, 0, errbuf);
			if (pd && (strlen(errbuf) > 0)) {
				 messagef(HAKA_LOG_WARNING, L"pcap", L"%s", errbuf);
			}
		}
		/* get a pcap descriptor from a pcap file */
		else if (strcmp(argv[0], "-f") == 0) {
			pd = pcap_open_offline(argv[1], errbuf);
		}
		/* unkonwn options */
		else  {
			messagef(HAKA_LOG_ERROR, L"pcap", L"unkown options");
			return 1;
		}

		if (!pd) {
			messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errbuf);
			return 1;
		}

		if (argc == 4) {
			/* open pcap savefile */
			if (strcmp(argv[2], "-o") == 0) {
				pf = pcap_dump_open(pd, argv[3]);
				if (!pf) {
					pcap_close(pd);
					messagef(HAKA_LOG_ERROR, L"pcap", L"unable to dump on %s", argv[3]);
					return 1;
				}
			}
			else {
				pcap_close(pd);
				messagef(HAKA_LOG_ERROR, L"pcap", L"output option should be -o");
				return 1;
			}
		}
	}
	else {
		messagef(HAKA_LOG_ERROR, L"pcap", L"specify a device (-i) or a pcap filename (-f)");
		return 1;
	}
	return 0;
}

static void cleanup()
{
	/* close pcap descritpors */
	if (pf)
		pcap_dump_close(pf);
	pcap_close(pd);
}

static int packet_receive(struct packet **pkt)
{
	struct pcap_pkthdr *header;
	struct packet *packet = NULL;
	const u_char *p;
	int ret;

	/* read packet */
	ret = pcap_next_ex(pd, &header, &p);

	if(ret == -1) {
		/* error while reading packet */
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(pd));
		return 1;
	}
	else if (ret == -2) {
		/* end of pcap file */
		return 1;
	}
	else {
		packet = malloc(sizeof(struct packet) + header->caplen);
		if (!packet) {
			return ENOMEM;
		}
		/* fill packet data structure */
		memcpy(packet->data, p, header->caplen);
		packet->header = *header;

		if (packet->header.caplen < packet->header.len)
		    messagef(HAKA_LOG_WARNING, L"pcap", L"packet truncated");

		*pkt = packet;
		return 0;
	}
}

static void packet_verdict(struct packet *pkt, filter_result result)
{
	/* dump capture in pcap file */
	if (pf && result == FILTER_ACCEPT)
		pcap_dump((u_char *)pf, &(pkt->header), pkt->data);
	free(pkt);
}

/* Skip the ethernet header */
#define ETHERNET_SIZE    14

static size_t packet_get_length(struct packet *pkt)
{
	return pkt->header.caplen - ETHERNET_SIZE;
}

static const uint8 *packet_get_data(struct packet *pkt)
{
	return (pkt->data + ETHERNET_SIZE);
}

static uint8 *packet_make_modifiable(struct packet *pkt)
{
	return (pkt->data + ETHERNET_SIZE);
}


/**
 * @defgroup Pcap Pcap
 * @brief Packet capturing using the libpcap.
 * @author Arkoon Network Security
 * @ingroup ExternPacketModule
 *
 * # Description
 *
 * The module uses the library pcap to read packets from a file or from an real network
 * device.
 *
 * To be able to capture packets on a real interface, the process need to be launched
 * with the proper access rights.
 *
 * # Initialization arguments
 *
 * ~~~~~~~~
 * ( -f <pcap file> | -i <interface name> ̀| -i any ) [ -o <pcap file> ]
 * ~~~~~~~~
 *
 * # Usage
 *
 * Module usage.
 *
 * ### Lua
 * ~~~~~~~~{.lua}
 * app.install("packet", module.load("packet-pcap", {"-f", "dump.pcap", "-o", "output.pcap"}))
 * ~~~~~~~~
 */
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
	make_modifiable: packet_make_modifiable,
	get_data:        packet_get_data
};

