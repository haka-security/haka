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
struct netmap_link_s
{
    char * input;
    char * output;
};
typedef struct netmap_link_s netmap_link_t;

#define MAX_LINKS 32
netmap_link_t links[MAX_LINKS];
int links_count = 0;

#define IFLN_DELIMITER ";"

/*
 * how many packets on this set of queues ?
 */
int
pkt_queued(struct nm_desc *d, int tx)
{
        u_int i, tot = 0;

        if (tx) {
                for (i = d->first_tx_ring; i <= d->last_tx_ring; i++) {
                        tot += nm_ring_space(NETMAP_TXRING(d->nifp, i));
                }
        } else {
                for (i = d->first_rx_ring; i <= d->last_rx_ring; i++) {
                        tot += nm_ring_space(NETMAP_RXRING(d->nifp, i));
                }
        }
        return tot;
}

static void cleanup()
{
	printf("cleanup\n");

}

static bool interface_link_is_shortcut( char * ifln )
{
    char * cursor;

    for ( cursor = ifln; *cursor ; cursor++)
    {
        if ( *cursor == IFLN_DELIMITER[0] )
            return true;
    }
    return false;
}

/*
 * get the interface link string and give input and output.
 * return the type of ifln.
 */

#define IFLN_IO_SEPERATOR '>'
#define IFLN_OI_SEPERATOR '<'
#define IFLN_IOOI_SEPERATOR '='

static int interface_link_get_in_and_out( char * ifln, netmap_link_t * nml, int idx )
{
    char * cursor;

    if ( !ifln || !nml || idx < 0 || idx >= MAX_LINKS )
        return 0;

    for ( cursor = ifln; *cursor ; cursor++ )
    {
        if ( *cursor == IFLN_IO_SEPERATOR )
        {
            *cursor = 0;
            nml[idx].input = strdup(ifln);
            nml[idx].output = strdup(cursor);
            return 1;
        }
        if ( *cursor == IFLN_OI_SEPERATOR )
        {
            *cursor = 0;
            nml[idx].input = strdup(cursor);
            nml[idx].output = strdup(ifln);
            return 1;
        }
        if ( *cursor == IFLN_IOOI_SEPERATOR )
        {
            *cursor = 0;
            nml[idx].input = strdup(ifln);
            nml[idx].output = strdup(cursor+1);
            idx++;
            if (idx >= MAX_LINKS)
            {
                messagef(HAKA_LOG_ERROR, L"netmap", L"Links table is full ");
                return 1;
            }
            nml[idx].input = strdup(cursor+1);
            nml[idx].output = strdup(ifln);
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
    if ( !orig )
    {
        messagef(HAKA_LOG_ERROR, L"netmap", L"links not found in configuration file" );
        return -1;
    }

    links_str = strdup(orig);
    // remove white space
    for ( cursor = links_str, idx = 0; *cursor ; cursor++ )
    {
        if ( *cursor == ' ' || *cursor == '\t' )
            idx++;
        else
            *(cursor - idx) = *cursor;
    }
    messagef(HAKA_LOG_INFO, L"netmap", L"links = %d", links_str );

    for ( idx = 0; idx < MAX_LINKS ; idx++ )
    {
        char * link, * token;

        token = strtok( idx==0?links_str:NULL , IFLN_DELIMITER);
        if ( !token ) // no more token
            break;

        link = strdup(token);
        if ( link )
        {
            int ret = 0, jdx;
            // if param is an interface:
            if ( interface_link_is_shortcut(link) )
            {
                char * temp;
                asprintf( &temp, "%s=%s", link, link );
                free(link);
                link = temp;
            }

            ret = interface_link_get_in_and_out( link, links, idx );
            if ( ret < 0 )
            {
                free(link);
                return EINVAL;
            }
            for ( jdx = 0 ; jdx < ret ; jdx++ )
            {
                messagef(HAKA_LOG_INFO, L"netmap", L"%d input: %s, output: %s", links_count+jdx, links[idx+jdx].input, links[idx+jdx].output);
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
	printf("cleanup_state\n");
}


static struct packet_module_state *init_state(int thread_id)
{
    struct packet_module_state *state;
    char * input = links[0].input;
    char * output = links[0].output;

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

    messagef(HAKA_LOG_INFO, L"netmap", L"packet queued [input(tx:%d,rx:%d),output(tx:%d,rx:%d)]",
             pkt_queued(state->idesc, 1), pkt_queued(state->idesc, 0),
             pkt_queued(state->odesc, 1), pkt_queued(state->odesc, 0));

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

    messagef(HAKA_LOG_INFO, L"netmap", L"poll pass successfully, packet queued [input(tx:%d,rx:%d),output(tx:%d,rx:%d)]",
             pkt_queued(state->idesc, 1), pkt_queued(state->idesc, 0),
             pkt_queued(state->odesc, 1), pkt_queued(state->odesc, 0));

    if ( state->pollfd.revents & POLLIN)
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

                messagef(HAKA_LOG_INFO, L"netmap", L" we can send the packet to haka filtering process ");

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

	if ( result == FILTER_ACCEPT)
	{
		const uint8 *data;
		size_t len;

		data = vbuffer_flatten( &orig_pkt->payload, &len);
		nm_inject( pkt->state->odesc, data, len );
        ioctl(pkt->state->odesc->fd, NIOCTXSYNC, NULL );
        messagef(HAKA_LOG_INFO, L"netmap", L"ACCEPT : packet %p of %d bytes sent to the output interface", data, len );
	}
    else
        messagef(HAKA_LOG_INFO, L"netmap", L"DROP");
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
