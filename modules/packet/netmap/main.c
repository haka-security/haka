/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define _GNU_SOURCE

#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/types.h>
#include <haka/parameters.h>
#include <haka/thread.h>
#include <haka/error.h>
#include <haka/container/list.h>
#include <haka/vbuffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

struct packet_module_state {
	struct nm_desc *idesc;
	struct nm_desc *odesc;
	struct pollfd pollfd;
	int index;
};

struct netmap_packet {
	struct packet               core_packet;
	struct packet_module_state *state;
};

/* Init parameters */
static char *input = NULL;
static char *output = NULL;

static void cleanup()
{
	printf("cleanup\n");

	if ( input )
		free(input);
	if ( output )
		free(output);
}

static int init(struct parameters *args)
{
	input = strdup(parameters_get_string(args, "input", NULL));
	if (!input)
		return EXIT_FAILURE;
	output = strdup(parameters_get_string(args, "output", NULL ));
	if (!output)
		return EXIT_FAILURE;

    messagef(HAKA_LOG_INFO, L"netmap", L"input: %s, output: %s", input, output);

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
	printf("cleanup_state\n");
}


static struct packet_module_state *init_state(int thread_id)
{
	printf("init_state\n");
	struct packet_module_state *state;

	state = malloc(sizeof(struct packet_module_state));
	if (!state) {
		error(L"memory error");
		return NULL;
	}

	state->idesc = nm_open( input, NULL, 0, NULL);
	if ( state->idesc == NULL )
    {
        messagef(HAKA_LOG_ERROR, L"netmap", L"cannot open %s\n", input);
		return NULL;
	}

	state->odesc = nm_open( output, NULL, NM_OPEN_NO_MMAP, state->idesc );
	if ( state->odesc == NULL )
	{
        messagef(HAKA_LOG_ERROR, L"netmap", L"cannot open %s\n", output);
		return NULL;
    }

	memset( &state->pollfd, 0, sizeof(state->pollfd));
	state->index = 0;

    messagef(HAKA_LOG_INFO, L"netmap", L"succeed to init state %p ", state );

	return state;
}

static int packet_do_receive(struct packet_module_state *state, struct packet **pkt)
{
    int err;

    if ( !state )
        return -1;

	state->pollfd.fd = state->idesc->fd;
	state->pollfd.events = POLLIN;
    state->pollfd.revents = 0;

    messagef(HAKA_LOG_INFO, L"netmap", L"polling on fd %d", state->pollfd.fd );

    err = poll( &state->pollfd, 1, -1);
    if ( err < 0 )
	{
        messagef(HAKA_LOG_ERROR, L"netmap", L"packet_do_receive poll fail ");
		return -1;
    }

    messagef(HAKA_LOG_INFO, L"netmap", L"packet_do_receive poll pass successfully with status %d", err );

	if (state->pollfd.revents & POLLIN)
	{
		int si = state->idesc->first_rx_ring;
		while ( si <= state->idesc->last_rx_ring )
		{
			struct netmap_ring * rxring = NETMAP_RXRING(state->idesc->nifp, si);
			struct netmap_slot * slot;
            int counter = 1, max = state->idesc->last_rx_ring - state->idesc->first_rx_ring + 1;

			if ( nm_ring_empty(rxring))
            {
                messagef(HAKA_LOG_INFO, L"netmap", L"rx ring %d/%d is empty ", counter, max);
				si++;
			}
			else
			{
				int space = 0;
				int idx;
				char * rxbuf;
				struct netmap_packet * nmpkt;

                messagef(HAKA_LOG_INFO, L"netmap", L"rx ring %d/%d is not empty ", counter, max);

				idx = rxring->cur;

				space = nm_ring_space(rxring);
				if ( space < 1 )
				{
                    messagef(HAKA_LOG_ERROR, L"netmap", L"Warning : no slot in ring but poll say there is data\n");
					return 0;
				}
				slot = &rxring->slot[idx];
				if ( slot->buf_idx < 2)
				{
                    messagef(HAKA_LOG_WARNING, L"netmap", L"wrong index rx[%d] = %d\n", idx, slot->buf_idx);
					sleep(2);
				}

				if ( slot->len > 2048 )
				{
                    messagef(HAKA_LOG_ERROR, L"netmap", L"wrong len %d rx[%d]\n", slot->len, idx );
					slot->len = 0;
					return 0;
				}

				rxbuf = NETMAP_BUF( rxring, slot->buf_idx );

				nmpkt = malloc(sizeof(struct netmap_packet));
				if (!nmpkt)
				{
                    messagef(HAKA_LOG_ERROR, L"netmap", L"cannot allocate memory");
					return ENOMEM;
				}
                if ( !vbuffer_create_from( &nmpkt->core_packet.payload, rxbuf, slot->len ) )
                {
                    messagef(HAKA_LOG_ERROR, L"netmap", L"vbuffer_create_from fail");
                    return 0;
                }

				nmpkt->state = state;

				idx = nm_ring_next( rxring, idx );

				rxring->head = rxring->cur = idx;

                *pkt = (struct packet *)nmpkt;

                messagef(HAKA_LOG_INFO, L"netmap", L" we can send the packet ");

//                state->index++;
			}
            counter++;
		}
	}

	return 0;
}

static void packet_verdict(struct packet *orig_pkt, filter_result result)
{
	struct netmap_packet * pkt = (struct netmap_packet *) orig_pkt;

	if ( !pkt )
		return;

    messagef(HAKA_LOG_INFO, L"netmap", L"packet_verdict\n");
	if ( result == FILTER_ACCEPT)
	{
		const uint8 *data;
		size_t len;
		data = vbuffer_flatten( &orig_pkt->payload, &len);
		nm_inject( pkt->state->odesc, data, len );
	}
}

static const char *packet_get_dissector(struct packet *orig_pkt)
{
	printf("packet_get_dissector\n");
	return NULL;
}

static uint64 packet_get_id(struct packet *orig_pkt)
{
	printf("packet_get_id\n");
	return 0;
}

static void packet_do_release(struct packet *orig_pkt)
{
	printf("packet_do_release\n");
}

static enum packet_status packet_getstate(struct packet *orig_pkt)
{
	printf("packet_getstate\n");
	return STATUS_NORMAL;
}

static struct packet *new_packet(struct packet_module_state *state, size_t size)
{
	struct packet *packet;

	printf("new_packet\n");
	packet = malloc(sizeof(struct packet));
	if (!packet) {
		error(L"Memory error");
		return NULL;
	}

	memset(packet, 0, sizeof(struct packet));

	if (!vbuffer_create_new(&packet->payload, size, true)) {
		assert(check_error());
		free(packet);
		return NULL;
	}

	return packet;
}

static bool send_packet(struct packet *orig_pkt)
{
	printf("send_packet\n");
	return true;
}

static size_t get_mtu(struct packet *pkt)
{
	return 1500;
}

static const struct time *get_timestamp(struct packet *orig_pkt)
{
	static struct time toto;
	return &toto;
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
