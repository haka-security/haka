/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Packet capture module.
 */

#ifndef _HAKA_PACKET_MODULE_H
#define _HAKA_PACKET_MODULE_H

#include <haka/module.h>
#include <haka/packet.h>
#include <haka/types.h>


/**
 * Filtering result.
 */
typedef enum {
	FILTER_ACCEPT, /**< Accept packet result. */
	FILTER_DROP    /**< Drop packet result. */
} filter_result;

struct packet_module_state; /**< Opaque state structure. */

/**
 * Packet module used to interact with the low-level packets. The module
 * will be used to receive packets and set a verdict on them. It also define
 * an interface to access the packet fields.
 */
struct packet_module {
	struct module    module; /**< Module structure. */

	/**
	 * Get if the module support multi-threading.
	 */
	bool           (*multi_threaded)();

	/**
	 * Get the capture mode.
	 */
	bool           (*pass_through)();

	/**
	 * Initialize the packet module state. This function will be called to create
	 * multiple states if the module supports multi-threading.
	 */
	struct packet_module_state *(*init_state)(int thread_id);

	/**
	 * Cleanup the packet module state.
	 */
	void           (*cleanup_state)(struct packet_module_state *state);

	/**
	 * Callback used to receive a new packet. This function should block until
	 * a packet is received.
	 *
	 * \returns Non zero in case of error.
	 */
	int            (*receive)(struct packet_module_state *state, struct packet **pkt);

	/**
	 * Apply a verdict on a received packet. The module should then apply this
	 * verdict on the underlying packet.
	 *
	 * \param pkt The received packet. After calling this function the packet address
	 * is never used again by the application but allow the module to free it if needed.
	 * \param result The verdict to apply to this packet.
	 */
	void           (*verdict)(struct packet *pkt, filter_result result);

	/**
	 * Get the identifier of a packet.
	 */
	uint64         (*get_id)(struct packet *pkt);

	/**
	 * Get the packet dissector.
	 */
	const char    *(*get_dissector)(struct packet *pkt);

	/**
	 * Release the packet and free its memory.
	 */
	void           (*release_packet)(struct packet *pkt);

	/**
	 * Get packet status.
	 */
	enum packet_status (*packet_getstate)(struct packet *pkt);

	/**
	 * Create a new packet.
	 */
	struct packet *(*new_packet)(struct packet_module_state *state, size_t size);

	/**
	 * Send a forged packet. This packet will be received again by Haka as
	 * a regular packet on the network.
	 */
	bool           (*send_packet)(struct packet *pkt);

	/**
	 * Get the mtu for a packet.
	 */
	size_t         (*get_mtu)(struct packet *pkt);

	/**
	 * Get the packet timestamp.
	 */
	const struct time *(*get_timestamp)(struct packet *pkt);
};

#endif /* _HAKA_PACKET_MODULE_H */
