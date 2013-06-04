/**
 * @file haka/packet.h
 * @brief Packet API
 * @author Pierre-Sylvain Desse
 *
 * The file contains the packet API functions.
 */

/**
 * @defgroup Packet Packet
 * @brief Packet API functions and structures.
 * @ingroup API_C
 */

#ifndef _HAKA_PACKET_H
#define _HAKA_PACKET_H

#include <stddef.h>
#include <haka/types.h>


/**
 * Opaque packet structure.
 * @ingroup Packet
 */
struct packet {
	void *lua_state;
};

/**
 * Get the length of a packet.
 * @param pkt Opaque packet.
 * @return Length of the packet data.
 * @ingroup Packet
 */
size_t       packet_length(struct packet *pkt);

/**
 * Get the data of a packet
 * @param pkt Opaque packet.
 * @return Address of the beginning of the packet data.
 * @ingroup Packet
 */
const uint8 *packet_data(struct packet *pkt);

/**
 * Get packet dissector to use.
 * @param pkt Opaque packet.
 * @return The dissector name.
 * @ingroup Packet
 */
const char  *packet_dissector(struct packet *pkt);

/**
 * Make a packet modifiable.
 * @param pkt Opaque packet.
 * @return Address of the beginning of the packet data.
 * @ingroup Packet
 */
uint8       *packet_data_modifiable(struct packet *pkt);

int          packet_resize(struct packet *pkt, size_t size);

/**
 * Drop a packet. The packet will be released and should not be used anymore
 * after this call.
 * @param pkt Opaque packet.
 */
void         packet_drop(struct packet *pkt);

/**
 * Accept a packet. The packet will be released and should not be used anymore
 * after this call.
 * @param pkt Opaque packet.
 */
void         packet_accept(struct packet *pkt);

#endif /* _HAKA_PACKET_H */

