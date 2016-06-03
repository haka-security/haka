/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Packet manipulation utilities.
 */

#ifndef HAKA_PACKET_H
#define HAKA_PACKET_H

#include <stddef.h>
#include <haka/types.h>
#include <haka/time.h>
#include <haka/time.h>
#include <haka/thread.h>
#include <haka/vbuffer.h>
#include <haka/lua/object.h>
#include <haka/lua/ref.h>


/** Opaque packet structure. */
struct packet {
	struct lua_object        lua_object; /**< \private */
	atomic_t                 ref;        /**< \private */
	struct vbuffer           payload;    /**< \private */
	struct lua_ref           userdata;
	struct lua_ref           next_dissector;
};

/** \cond */
struct capture_module_state;
struct engine_thread;
/** \endcond */

/**
 * Packet status.
 */
enum packet_status {
	STATUS_NORMAL, /**< Packet captured by Haka. */
	STATUS_FORGED, /**< Packet forged. */
	STATUS_SENT,   /**< Packet already sent on the network. */
};

/**
 * Initialize packet internals for a given thread.
 */
bool               packet_init(struct capture_module_state *state);

/**
 * Increment packet ref count.
 */
void               packet_addref(struct packet *pkt);

/**
 * Decrement packet ref count and release it if needed.
 */
bool               packet_release(struct packet *pkt);

/**
 * Create a new packet.
 */
struct packet     *packet_new(size_t size);

/**
 * Get the timestamp of the packet.
 */
const struct time *packet_timestamp(struct packet *pkt);

/**
 * Get the id of the packet.
 */
uint64             packet_id(struct packet *pkt);

/**
 * Get the packet payload.
 */
struct vbuffer    *packet_payload(struct packet *pkt);

/**
 * Get the packet dissector to use.
 */
const char        *packet_dissector(struct packet *pkt);

/**
 * Drop the packet.
 */
void               packet_drop(struct packet *pkt);

/**
 * Accept a packet and send it on the network.
 */
void               packet_accept(struct packet *pkt);

/**
 * Send a packet. The packet will re-enter Haka to be filtered
 * like a new packet.
 */
bool               packet_send(struct packet *pkt);

/**
 * Wait for some packet to be availabe.
 */
int                packet_receive(struct engine_thread *engine, struct packet **pkt);

/**
 * Get the packet mtu.
 */
size_t             packet_mtu(struct packet *pkt);

/**
 * Get the packet state.
 */
enum packet_status packet_state(struct packet *pkt);

/**
 * Packet capture mode.
 */
enum capture_mode {
	MODE_NORMAL,      /**< Normal mode (read/write). */
	MODE_PASSTHROUGH, /**< Listen mode (read-only). */
};

/**
 * Current packet capture mode.
 */
enum capture_mode   capture_mode();

extern struct time_realm network_time;

#endif /* HAKA_PACKET_H */
