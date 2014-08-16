/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Author : Lahoudere Fabien (fabien.lahoudere@openwide.fr */

#define _GNU_SOURCE

#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/parameters.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/container/list.h>
#include <haka/vbuffer.h>
#include <haka/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#include "libnetmap.h"

/* Init parameters */
struct netmap_link_s
{
	int id;
	char * input;
	char * output;
	struct nm_desc *idesc;
	struct nm_desc *odesc;
	int related_link;
};
typedef struct netmap_link_s netmap_link_t;

void show_link(netmap_link_t * link)
{
	if (!link)
		messagef(HAKA_LOG_INFO, L"netmap", L"link is NULL");
	else
		messagef(HAKA_LOG_INFO, L"netmap",
				 L"link = %p { input(%p=>%s), output(%p=>%s), related link = %d }",
				 link, link->idesc, link->input, link->odesc, link->output, link->related_link);
}

#define MAX_LINKS 32
netmap_link_t links[MAX_LINKS];
int links_count = 0;
uint32_t packet_counter = 0;

#define IFLN_DELIMITER ";"

// haka structure definition
struct packet_module_state {
	struct pollfd *pfds;
	int index;
	netmap_link_t *default_link;
};

struct netmap_packet {
	struct packet core_packet;
    uint32_t id;
    struct time timestamp;
	struct vbuffer data;
	struct vbuffer_iterator select;
	netmap_link_t *link;
	short protocol;
	enum packet_status status;
};

static void cleanup()
{
	int idx;
	for ( idx = 0; idx < links_count ; idx++ ) {
		if ( links[idx].input )
			free(links[idx].input);
		if ( links[idx].output )
			free(links[idx].output);
		if (links[idx].idesc)
			nm_close(links[idx].idesc);
		if (links[idx].odesc)
			nm_close(links[idx].odesc);
	}
}

static bool interface_link_is_shortcut( char * ifln )
{
	char * cursor;

	for (cursor = ifln; *cursor ; cursor++) {
		if (*cursor == IFLN_DELIMITER[0])
			return true;
	}
	return false;
}

/*
 * get the interface link string and give input and output.
 */

#define IFLN_IO_SEPERATOR '>'
#define IFLN_OI_SEPERATOR '<'
#define IFLN_IOOI_SEPERATOR '='

static int interface_link_get_in_and_out(char * ifln, netmap_link_t * nml, int idx)
{
	char * cursor;

	if (!ifln || !nml || idx < 0 || idx >= MAX_LINKS)
		return 0;

	for (cursor = ifln; *cursor ; cursor++) {
		nml[idx].id = idx;
		if (*cursor == IFLN_IO_SEPERATOR) {
			*cursor = 0;
			nml[idx].input = strdup(ifln);
			nml[idx].output = strdup(cursor);
			nml[idx].related_link = idx;
			return 1;
		}
		if (*cursor == IFLN_OI_SEPERATOR) {
			*cursor = 0;
			nml[idx].input = strdup(cursor);
			nml[idx].output = strdup(ifln);
			nml[idx].related_link = idx;
			return 1;
		}
		if (*cursor == IFLN_IOOI_SEPERATOR) {
			*cursor = 0;
			nml[idx].input = strdup(ifln);
			nml[idx].output = strdup(cursor+1);
			nml[idx].related_link = idx+1;
			idx++;
			if (idx >= MAX_LINKS) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"Links table is full ");
				return 1;
			}
			nml[idx].id = idx;
			nml[idx].input = strdup(cursor+1);
			nml[idx].output = strdup(ifln);
			nml[idx].related_link = idx-1;
			return 2;
		}
	}
	return -1;
}


static int init(struct parameters *args)
{
	int idx;
	char * links_str, * cursor;
	const char * orig;

	orig = parameters_get_string(args, "links", NULL);
	if (!orig) {
		messagef(HAKA_LOG_ERROR, L"netmap", L"links not found in configuration file");
		return -1;
	}

	links_str = strdup(orig);
	// remove white space
	for (cursor = links_str, idx = 0; *cursor ; cursor++) {
		if ( *cursor == ' ' || *cursor == '\t' )
			idx++;
		else
			*(cursor - idx) = *cursor;
	}
	messagef(HAKA_LOG_INFO, L"netmap", L"links = %s", links_str);

	while (links_count < MAX_LINKS) {
		char * link, * token;

		token = strtok(links_count==0?links_str:NULL , IFLN_DELIMITER);
		if (!token) // no more token
			break;

		link = strdup(token);
		if (link) {
			int ret = 0, idx;
			// if param is an interface:
			if (interface_link_is_shortcut(link)) {
				char * temp;
				ret = asprintf(&temp, "%s=%s", link, link);
				if (ret < 0)
					return ENOMEM;
				free(link);
				link = temp;
			}

			ret = interface_link_get_in_and_out(link, links, links_count);
			if (ret < 0) {
				free(link);
				return EINVAL;
			}
			for ( idx = 0 ; idx < ret ; idx++ ) {
				messagef(HAKA_LOG_INFO, L"netmap", L"%d input: %s, output: %s",
						 links_count+idx, links[links_count+idx].input,
						links[links_count+idx].output);
			}
			links_count+=ret;
		}
	}
	free(links_str);

	return 0;
}

static bool multi_threaded()
{
	return false;
}

static bool pass_through()
{
	return false;
}

static void cleanup_state(struct packet_module_state *state)
{
	if (state) {
		if (state->pfds)
			free(state->pfds);
	}
}


static struct packet_module_state *init_state(int thread_id)
{
	struct packet_module_state *state;
	int idx;

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		error(L"memory error");
		return NULL;
	}

	state->pfds = malloc(links_count * sizeof(struct pollfd));
	if (!state->pfds) {
		error(L"memory error");
		return NULL;
	}

	for (idx = 0; idx < links_count ; idx++) {
		if (links[idx].related_link == idx - 1) {
			links[idx].idesc = links[idx-1].odesc;
			links[idx].odesc = links[idx-1].idesc;
			state->pfds[idx].fd = links[idx].idesc->fd;
			state->pfds[idx].events = POLLIN;
			state->pfds[idx].revents = 0;
		}
		else {
			links[idx].idesc = nm_open(links[idx].input, NULL, 0, NULL);
			if (links[idx].idesc == NULL) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"cannot open %s",
						 links[idx].input);
				return NULL;
			}
			state->pfds[idx].fd = links[idx].idesc->fd;
			state->pfds[idx].events = POLLIN;
			state->pfds[idx].revents = 0;

			links[idx].odesc = nm_open(links[idx].output, NULL, NM_OPEN_NO_MMAP,
									   links[idx].idesc);
			if (links[idx].odesc == NULL) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"cannot open %s",
						 links[idx].output);
				return NULL;
			}
		}
	}

	state->index = 0;

	messagef(HAKA_LOG_INFO, L"netmap",
			 L"succeed to init state(%p) for %d links ", state, links_count);

	return state;
}

static bool packet_build_payload(struct netmap_packet *packet)
{
	struct vbuffer_sub sub;

	// remove etthernet header from analysis
	vbuffer_sub_create(&sub, &packet->data, sizeof(struct ethhdr), ALL);

	return vbuffer_select(&sub, &packet->core_packet.payload, &packet->select);
}


static int get_buffer_from_link(netmap_link_t * link, struct netmap_packet ** pkt)
{
	int si, counter=0;
	for (si = link->idesc->first_rx_ring; si <= link->idesc->last_rx_ring; si++) {
		struct netmap_ring * rxring = NETMAP_RXRING(link->idesc->nifp, si);
		struct netmap_slot * slot;

		if (nm_ring_empty(rxring)) {
			messagef(HAKA_LOG_INFO, L"netmap", L"rx ring %d is empty ", si);
		}
		else {
			int space = 0;
			int idx;
			char * rxbuf;
			struct netmap_packet * nmpkt;

			idx = rxring->cur;

			space = nm_ring_space(rxring);
			if (space < 1) {
				messagef(HAKA_LOG_ERROR, L"netmap",
						 L"Warning : no slot in ring but poll say there is data\n");
				return 0;
			}
			slot = &rxring->slot[idx];
			if (slot->buf_idx < 2) {
				messagef(HAKA_LOG_WARNING,
						 L"netmap", L"wrong index rx[%d] = %d\n", idx,
						 slot->buf_idx);
				sleep(2);
			}

			if (slot->len > 2048) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"wrong len %d rx[%d]\n",
						 slot->len, idx);
				slot->len = 0;
				return 0;
			}

			rxbuf = NETMAP_BUF(rxring, slot->buf_idx);

			nmpkt = malloc(sizeof(struct netmap_packet));
			if (!nmpkt) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"cannot allocate memory");
				return ENOMEM;
			}

            time_gettimestamp(&nmpkt->timestamp);
            nmpkt->id = packet_counter++;

			if (!vbuffer_create_from(&nmpkt->data, rxbuf, slot->len)) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"vbuffer_create_from fail");
				return 0;
			}

			nmpkt->link = link;

			idx = nm_ring_next(rxring, idx);

			rxring->head = rxring->cur = idx;

			nmpkt->protocol = ntohs(((struct ethhdr*)rxbuf)->h_proto);

			if (!packet_build_payload(nmpkt)) {
				messagef(HAKA_LOG_ERROR, L"netmap", L"packet_build_payload fail");
				vbuffer_release(&nmpkt->data);
				free(nmpkt);
				return ENOMEM;
			}

			nmpkt->status = STATUS_NORMAL;

			*pkt = nmpkt;
		}
		counter++;
	}
	return counter;
}


static int packet_do_receive(struct packet_module_state *state,
							 struct packet **pkt)
{
	int err,idx;

	if (!state)
		return -1;
#ifdef NMDEBUG
	for (idx = 0 ; idx < links_count ; idx++) {
		messagef(HAKA_LOG_INFO, L"netmap",
				 L"[%d] packet queued {input(tx:%d,rx:%d),output(tx:%d,rx:%d)}",
				 idx,
				 pkt_queued(links[idx].idesc, 1),
				 pkt_queued(links[idx].idesc, 0),
				 pkt_queued(links[idx].odesc, 1),
				 pkt_queued(links[idx].odesc, 0));
	}
#endif
	// reset all poll revents
	for (idx = 0 ; idx < links_count ; idx++) {
		state->pfds[idx].events = POLLIN;
		state->pfds[idx].revents = 0;
	}

#ifdef NMDEBUG
	messagef(HAKA_LOG_INFO, L"netmap", L"polling %d links", links_count);
#endif

	// wait for events
	err = poll(state->pfds, links_count, -1);
	if (err < 0) {
		messagef(HAKA_LOG_ERROR, L"netmap", L"packet_do_receive poll fail");
		return -1;
	}
	else if (err == 0) {
		messagef(HAKA_LOG_ERROR, L"netmap", L"timeout");
		return -1;
	}

	// treat all events
	for (idx = 0 ; idx < links_count ; idx++) {
#ifdef NMDEBUG
		messagef(HAKA_LOG_INFO,
				 L"netmap", L"[%d] status = %d - packet queued {input(tx:%d,rx:%d),output(tx:%d,rx:%d)",
				 idx, state->pfds[idx].revents,
				 pkt_queued(links[idx].idesc, 1),
				 pkt_queued(links[idx].idesc, 0),
				 pkt_queued(links[idx].odesc, 1),
				 pkt_queued(links[idx].odesc, 0));
#endif

		if (state->pfds[idx].revents & POLLIN) {
            err = get_buffer_from_link(&links[idx], (struct netmap_packet **)pkt);
			if (err < 0)
				return err;
		}
	}

	return 0;
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	struct netmap_packet *pkt = (struct netmap_packet *)orig_pkt;

	if (!pkt)
		return;

	if (vbuffer_isvalid(&pkt->data)) {

        vbuffer_restore(&pkt->select, &pkt->core_packet.payload, false);

		if (result == FILTER_ACCEPT) {
			const uint8 *data;
			size_t len;

			data = vbuffer_flatten(&pkt->data, &len);
			if (pkt->link->odesc) {
				nm_inject(pkt->link->odesc, data, len);
				ioctl(pkt->link->odesc->fd, NIOCTXSYNC, NULL);
#ifdef NMDEBUG
				messagef(HAKA_LOG_INFO, L"netmap",
						 L"ACCEPT : packet %p of %d bytes sent to the output interface %s",
						 data, len, pkt->link->output );
#endif
			}
			else {
				messagef(HAKA_LOG_ERROR, L"netmap", L"packet_verdict : pkt->link->odesc is NULL");
				show_link(pkt->link);
			}
			pkt->status = STATUS_SENT;
		}
		else
			messagef(HAKA_LOG_INFO, L"netmap", L"DROP");
		vbuffer_clear(&pkt->data);
	}
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	char * dissector;
	struct netmap_packet *pkt = (struct netmap_packet*)orig_pkt;

	switch (pkt->protocol) {
		case 0x0800:
			dissector = "ipv4";
			break;
		default:
			dissector = NULL;
			break;
	}

	return dissector;
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
    return ((struct netmap_packet*)orig_pkt)->id;
}

static void packet_do_release(struct packet *orig_pkt)
{
	struct netmap_packet *pkt = (struct netmap_packet*)orig_pkt;

	if (vbuffer_isvalid(&pkt->data)) {
		packet_verdict(orig_pkt, FILTER_DROP);
	}

	vbuffer_release(&pkt->core_packet.payload);
	vbuffer_release(&pkt->data);
	free(pkt);
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	return ((struct netmap_packet*)orig_pkt)->status;
}

static struct packet *new_packet(struct packet_module_state *state, size_t size)
{
	struct netmap_packet *packet;

	packet = malloc(sizeof(struct netmap_packet));
	if (!packet) {
		error(L"Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct packet));

	if (!vbuffer_create_new(&packet->data, size, true)) {
		assert(check_error());
		free(packet);
		return NULL;
	}

	packet->link = &links[links_count-1];

	packet->protocol = 0x00;

	if (!packet_build_payload(packet)) {
		messagef(HAKA_LOG_ERROR, L"netmap", L"packet_build_payload fail");
		vbuffer_release(&packet->data);
		free(packet);
		return NULL;
	}

	packet->status = STATUS_NORMAL;

	return (struct packet*)packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	struct netmap_packet *pkt = (struct netmap_packet *)orig_pkt;

	if (!pkt)
		return false;

	if (vbuffer_isvalid(&pkt->data)) {

		const uint8 *data;
		size_t len;
        vbuffer_restore(&pkt->select, &pkt->core_packet.payload, false);

		data = vbuffer_flatten( &pkt->data, &len);
		if (pkt->link->odesc) {
			nm_inject(pkt->link->odesc, data, len);
			ioctl(pkt->link->odesc->fd, NIOCTXSYNC, NULL);
		}
		else {
			messagef(HAKA_LOG_ERROR, L"netmap", L"send_packet : pkt->link->odesc is NULL");
			show_link(pkt->link);
		}
		pkt->status = STATUS_SENT;
		vbuffer_clear(&pkt->data);
	}

	return true;
}

static size_t get_mtu(struct packet *pkt)
{
	return 1500;
}

static const struct time *get_timestamp(struct packet *orig_pkt)
{
    return &((struct netmap_packet *)orig_pkt)->timestamp;
}

static bool is_realtime()
{
    return false;
}

struct packet_module HAKA_MODULE = {
	module: {
	type:        MODULE_PACKET,
	name:        L"Netmap Module",
		description: L"Netmap packet module",
		api_version: HAKA_API_VERSION,
		init:        init,
		cleanup:     cleanup
},
		multi_threaded:  multi_threaded,
		pass_through:    pass_through,
        is_realtime:     is_realtime,
		init_state:      init_state,
		cleanup_state:   cleanup_state,
		receive:         packet_do_receive,
		verdict:         packet_verdict,
		get_id:          packet_get_id,
		get_dissector:   packet_get_dissector,
		release_packet:  packet_do_release,
		packet_getstate: packet_getstate,
		new_packet:      new_packet,
		send_packet:     send_packet,
		get_mtu:         get_mtu,
		get_timestamp:   get_timestamp
};
