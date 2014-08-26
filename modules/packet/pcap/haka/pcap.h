/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_PCAP_TCP_H
#define _HAKA_PCAP_TCP_H

/* snapshot length - a value of 65535 is sufficient to get all
 * of the packet's data on most networks (from pcap man page)
 */
#define SNAPLEN 65535

struct pcap_capture {
	pcap_t       *pd;
	int           link_type;
	FILE         *file;
	size_t        file_size;
	struct time   last_progress;
};

/*
 * Packet headers
 */

struct linux_sll_header {
	uint16     type;
	uint16     arphdr_type;
	uint16     link_layer_length;
	uint64     link_layer_address;
	uint16     protocol;
} PACKED;

int  get_link_type_offset(int link_type);
int  get_protocol(int link_type, struct vbuffer *data, size_t *data_offset);

#endif /* _HAKA_PCAP_TCP_H */
