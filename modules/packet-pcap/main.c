
#include <haka/packet_module.h>
#include <haka/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pcap.h>
#include <string.h>

/* snapshot length - a value of 65535 is sufficient to get all
 * of the packet's data on most networks (from pcap man page)
 */
#define SNAPLEN 65535

struct packet {
	struct pcap_pkthdr header;
	u_char data[0];
};

struct pcap_t {
	pcap_t        *pd;
	pcap_dumper_t *pf;
	size_t         link_hdr_len;
};

static struct pcap_t pcap;


static void cleanup()
{
	/* close pcap descriptors */
	if (pcap.pf) {
		pcap_dump_close(pcap.pf);
		pcap.pf = NULL;
	}
	pcap_close(pcap.pd);
	pcap.pd = NULL;
}

static int init(int argc, char *argv[])
{
	if (argc == 2 || argc == 4) {
		int linktype;
		char errbuf[PCAP_ERRBUF_SIZE];
		bzero(errbuf, PCAP_ERRBUF_SIZE);

		/* get a pcap descriptor from device */
		if (strcmp(argv[0], "-i") == 0) {
			pcap.pd = pcap_open_live(argv[1], SNAPLEN, 1, 0, errbuf);
			if (pcap.pd && (strlen(errbuf) > 0)) {
				 messagef(HAKA_LOG_WARNING, L"pcap", L"%s", errbuf);
			}
		}
		/* get a pcap descriptor from a pcap file */
		else if (strcmp(argv[0], "-f") == 0) {
			pcap.pd = pcap_open_offline(argv[1], errbuf);
		}
		/* unkonwn options */
		else  {
			messagef(HAKA_LOG_ERROR, L"pcap", L"unkown options");
			return 1;
		}

		if (!pcap.pd) {
			messagef(HAKA_LOG_ERROR, L"pcap", L"%s", errbuf);
			return 1;
		}

		if (argc == 4) {
			/* open pcap savefile */
			if (strcmp(argv[2], "-o") == 0) {
				pcap.pf = pcap_dump_open(pcap.pd, argv[3]);
				if (!pcap.pf) {
					pcap_close(pcap.pd);
					messagef(HAKA_LOG_ERROR, L"pcap", L"unable to dump on %s", argv[3]);
					return 1;
				}
			}
			else {
				pcap_close(pcap.pd);
				messagef(HAKA_LOG_ERROR, L"pcap", L"output option should be -o");
				return 1;
			}
		}

		// Determine the datalink layer type.
		if ((linktype = pcap_datalink(pcap.pd)) < 0)
		{
			messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(pcap.pd));
			cleanup();
			return 1;
		}

		// Set the datalink layer header size.
		switch (linktype)
		{
		case DLT_NULL:
			pcap.link_hdr_len = 4;
			break;

		case DLT_LINUX_SLL:
			pcap.link_hdr_len = 16;
			break;

		case DLT_EN10MB:
			pcap.link_hdr_len = 14;
			break;

		case DLT_SLIP:
		case DLT_PPP:
			pcap.link_hdr_len = 24;
			break;

		default:
			messagef(HAKA_LOG_ERROR, L"pcap", L"%s", "unsupported data link");
			cleanup();
			return 1;
		}
	}
	else {
		messagef(HAKA_LOG_ERROR, L"pcap", L"specify a device (-i) or a pcap filename (-f)");
		return 1;
	}
	return 0;
}

static int packet_receive(struct packet **pkt)
{
	struct pcap_pkthdr *header;
	struct packet *packet = NULL;
	const u_char *p;
	int ret;

	/* read packet */
	ret = pcap_next_ex(pcap.pd, &header, &p);

	if(ret == -1) {
		/* error while reading packet */
		messagef(HAKA_LOG_ERROR, L"pcap", L"%s", pcap_geterr(pcap.pd));
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
	if (pcap.pf && result == FILTER_ACCEPT)
		pcap_dump((u_char *)pcap.pf, &(pkt->header), pkt->data);
	free(pkt);
}

static size_t packet_get_length(struct packet *pkt)
{
	return pkt->header.caplen - pcap.link_hdr_len;
}

static const uint8 *packet_get_data(struct packet *pkt)
{
	return (pkt->data + pcap.link_hdr_len);
}

static uint8 *packet_make_modifiable(struct packet *pkt)
{
	return (pkt->data + pcap.link_hdr_len);
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

