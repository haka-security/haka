/**
 * @file packet_module.h
 * @brief Packet module interface
 * @author Pierre-Sylvain Desse
 *
 * The file contains the description of a haka packet module.
 */

#ifndef _HAKA_PACKET_MODULE_H
#define _HAKA_PACKET_MODULE_H

#include <haka/module.h>
#include <haka/packet.h>
#include <haka/types.h>

/**
 * @defgroup ExternPacketModule Packet
 * @brief External packet module documentation.
 * @ingroup ExternModule
 */


/**
 * Constant value to be used to communicate with the module in order
 * to accept or deny a packet.
 * @ingroup Packet
 */
typedef enum {
	FILTER_ACCEPT, /**< Accept the packet */
	FILTER_DROP /**< Deny the packet. The packet will be lost. */
} filter_result;

/**
 * Opaque state structure.
 */
struct packet_module_state;

/**
 * @brief Packet module structure
 *
 * Packet module used to interact with the low-level packets. The module
 * will be used to receive packets and set a verdict on them. It also define
 * an interface to access the packet fields.
 * @ingroup Module
 */
struct packet_module {
	struct module    module; /**< Module definition */

	/**
	 * Does this module supports multi-threading.
	 */
	bool           (*multi_threaded)();

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
	 * @param state Packet module state.
	 * @param pkt Pointer filled with the received packet.
	 * @return Non zero in case of error.
	 */
	int            (*receive)(struct packet_module_state *state, struct packet **pkt);

	/**
	 * Apply a verdict on a received packet. The module should then apply this
	 * verdict on the underlying packet.
	 * @param state Packet module state.
	 * @param pkt The received packet. After calling this funciton the packet
	 * address is never used again by the application allow the module to free
	 * it if needed.
	 * @param result The verdict to apply to this packet. @see filter_result
	 */
	void           (*verdict)(struct packet *pkt, filter_result result);

	/**
	 * Get the length of a packet.
	 * @param pkt The received opaque packet.
	 * @return The size of this packet.
	 */
	size_t         (*get_length)(struct packet *pkt);

	/**
	 * Make the packet modifiable.
	 * @param pkt The received opaque packet.
	 * @return 0 if success.
	 */
	uint8         *(*make_modifiable)(struct packet *pkt);

	/**
	 * Get the data of a packet
	 * @param pkt The received opaque packet.
	 * @return The address of the beginning of the packet data.
	 */
	const uint8   *(*get_data)(struct packet *pkt);
};

#endif /* _HAKA_PACKET_MODULE_H */

