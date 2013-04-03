/**
 * @file packet.h
 * @brief Packet API
 * @author Pierre-Sylvain Desse
 *
 * The file contains the packet API functions.
 */

/**
 * @defgroup Packet Packet API
 * @brief Packet API functions and structures.
 */

#ifndef _HAKA_PACKET_H
#define _HAKA_PACKET_H

#include <stddef.h>


/**
 * Opaque packet structure.
 * @ingroup Packet
 */
struct packet;

/**
 * Get the length of a packet.
 * @param pkt Opaque packet.
 * @return Length of the packet data.
 * @ingroup Packet
 */
size_t      packet_length(struct packet *pkt);

/**
 * Get the data of a packet
 * @param pkt Opaque packet.
 * @return Address of the beginning og the packlet data.
 * @ingroup Packet
 */
const char *packet_data(struct packet *pkt);

#endif /* _HAKA_PACKET_H */

