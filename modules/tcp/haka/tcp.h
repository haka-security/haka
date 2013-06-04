/**
 * file tcp.h
 * @brief  TCP Dissector API
 * @author Mehdi Talbi
 *
 * The file contains the TCP disscetor API functions and structures definition
 */

/**
 * @defgroup TCP TCP
 * @brief TCP dissector API functions and structures.
 * @ingroup ExternProtocolModule
 */

#ifndef _HAKA_PROTO_TCP_H
#define _HAKA_PROTO_TCP_H

#include <haka/types.h>
#include <haka/ipv4.h>

#define SWAP_TO_TCP(type, x)            SWAP_TO_BE(type, x)
#define SWAP_FROM_TCP(type, x)          SWAP_FROM_BE(type, x)
#define TCP_GET_BIT(type, v, i)         GET_BIT(SWAP_FROM_BE(type, v), i)
#define TCP_SET_BIT(type, v, i, x)      SWAP_TO_BE(type, SET_BIT(SWAP_FROM_BE(type, v), i, x))
#define TCP_GET_BITS(type, v, r)        GET_BITS(SWAP_FROM_BE(type, v), r)
#define TCP_SET_BITS(type, v, r, x)     SWAP_TO_BE(type, SET_BITS(SWAP_FROM_BE(type, v), r, x))

#define TCP_CHECK(tcp, ...)             if (!(tcp) || !(tcp)->packet) { error(L"invalid tcp packet"); return __VA_ARGS__; }

#define TCP_FLAGS_BITS                  0, 8
#define TCP_FLAGS_START                 13
#define TCP_HDR_LEN                     2 /* Header length is a multiple of 4 bytes */

#define TCP_PROTO 6

struct tcp_header {
	uint16    srcport;
	uint16    dstport;
	uint32	  seq;
	uint32	  ack_seq;
#ifdef HAKA_LITTLEENDIAN
	uint16    res:4,
	          hdr_len:4,
	          fin:1,
	          syn:1,
	          rst:1,
	          psh:1,
	          ack:1,
	          urg:1,
	          ecn:1,
	          cwr:1;
#else
	unint16	  hdr_len:4,
	          res:4,
	          cwr:1,
	          ece:1,
	          urg:1,
	          ack:1,
	          psh:1,
	          rst:1,
	          syn:1,
	          fin:1;
#endif
	uint16	  window_size;
	uint16	  checksum;
	uint16	  urgent_pointer;
};


/**
 * TCP opaque structure
 * @ingroup TCP
 */
struct tcp {
	struct ipv4         *packet;
	struct tcp_header   *header;
	bool                 modified:1;
	bool                 invalid_checksum:1;
};

/**
 * @brief Dissect TCP header
 * @param packet IPv4 structure
 * @return Pointer to TCP structure
 * @ingroup TCP
 */
struct tcp *tcp_dissect(struct ipv4 *packet);

/**
 * @brief Forge TCP packet
 * @param packet TCP structure
 * @ingroup TCP
 */
struct ipv4 *tcp_forge(struct tcp *packet);


/**
 * @brief Release TCP structure memory allocation
 * @param packet TCP structure
 * @ingroup TCP
 */
void tcp_release(struct tcp *packet);

/**
 * @brief Make TCP data modifiable
 * @param packet TCP structure
 * @ingroup TCP
 */
void tcp_pre_modify(struct tcp *packet);

/**
 * @brief Compute TCP checksum according to RFC #1071
 * @param packet TCP structure
 * @ingroup TCP
 */
void tcp_compute_checksum(struct tcp *packet);

/**
 * @brief Verify TCP checksum
 * @param packet TCP structure
 * @return true if checksum is valid and false otherwise
 * @ingroup TCP
 */
bool tcp_verify_checksum(const struct tcp *packet);

/**
 * Get TCP payload data.
 * @param packet TCP structure
 * @return Pointer to the tcp payload data.
 * @ingroup TCP
 */
const uint8 *tcp_get_payload(const struct tcp *packet);

/**
 * Get TCP modifiable payload data.
 * @param packet TCP structure
 * @return Pointer to the tcp payload data.
 * @ingroup TCP
 */
uint8 *tcp_get_payload_modifiable(struct tcp *packet);

/**
 * Get TCP payload length.
 * @param packet TCP structure
 * @return The TCP payload size.
 * @ingroup TCP
 */
size_t tcp_get_payload_length(const struct tcp *packet);

uint8 *tcp_resize_payload(struct tcp *packet, size_t size);

/**
 * Drop the TCP packet
 * @param packet TCP structure
 * @ingroup TCP
 */
void tcp_action_drop(struct tcp *packet);

void tcp_flush(struct tcp *packet);

/**
 * Get if the packet is valid and can continue to be processed.
 * @param packet TCP structure
 * @return True if the packet is valid.
 * @ingroup TCP
 */
bool tcp_valid(struct tcp *packet);

#define TCP_GETSET_FIELD(type, field) \
	INLINE type tcp_get_##field(const struct tcp *tcp) { TCP_CHECK(tcp, 0); return SWAP_FROM_TCP(type, tcp->header->field); } \
	INLINE void tcp_set_##field(struct tcp *tcp, type v) { TCP_CHECK(tcp); tcp_pre_modify(tcp); tcp->header->field = SWAP_TO_TCP(type, v); }

/**
 * @fn uint16 tcp_get_srcport(const struct tcp *tcp)
 * @brief Get TCP source port
 * @param tcp TCp structure
 * @return TCP source port value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_srcport(struct tcp *tcp, uint16 v)
 * @brief Set TCP version
 * @param tcp TCP structure
 * @param v Source port value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint16, srcport);


/**
 * @fn uint8 tcp_get_dstport(const struct tcp *tcp)
 * @brief Get TCP destination port
 * @param tcp TCP structure
 * @return TCP version value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_dstport(struct tcp *tcp, uint16 v)
 * @brief Set TCP destination port
 * @param tcp TCP structure
 * @param v TCP destination port value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint16, dstport);


/**
 * @fn uint32 tcp_get_seq(const struct tcp *tcp)
 * @brief Get TCP sequence number
 * @param tcp TCP structure
 * @return TCP squence number value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_seq(struct tcp *tcp, uint32 v)
 * @brief Set TCP sequence number
 * @param tcp TCP structure
 * @param v Sequence number value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint32, seq);

/**
 * @fn uint8 tcp_get_ack_seq(const struct tcp *tcp)
 * @brief Get TCP version
 * @param tcp TCP structure
 * @return TCP version value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_ack_seq(struct tcp *tcp, uint32 v)
 * @brief Set TCP ACK sequence value
 * @param tcp TCP structure
 * @param v TCP ACK sequence value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint32, ack_seq);

/**
 * @fn uint8 tcp_get_res(const struct tcp *tcp)
 * @brief Get TCP reserved field
 * @param tcp TCP structure
 * @return TCP reserved value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_res(struct tcp *tcp, uint8 v)
 * @brief Set TCP reserved field
 * @param tcp TCP structure
 * @param v Reserved value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint8, res);

/**
 * @fn uint8 tcp_get_window_size(const struct tcp *tcp)
 * @brief Get TCP window size
 * @param tcp TCP structure
 * @return TCP window size value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_window_size(struct tcp *tcp, uint16 v)
 * @brief Set TCP window size
 * @param tcp TCP structure
 * @param v Window size value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint16, window_size);

/**
 * @fn uint8 tcp_get_urgent_pointer(const struct tcp *tcp)
 * @brief Get TCP version
 * @param tcp TCP structure
 * @return TCP version value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_urgent_pointer(struct ipv4 *ip, uint16 v)
 * @brief Set TCP urgent pointer
 * @param tcp TCP structure
 * @param v Version value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint16, urgent_pointer);


/**
 * @fn uint16 tcp_get_checksum(const struct tcp *tcp)
 * @brief Get TCP cheksum
 * @param tcp TCP structure
 * @return TCP checksum value
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_checksum(struct tcp *tcp, uint16 v)
 * @brief Set TCP cheksum
 * @param tcp TCP structure
 * @param v Checksum value
 * @ingroup TCP
 */
TCP_GETSET_FIELD(uint16, checksum);


/**
 * @fn uint8 tcp_get_hdr_len(const struct tcp *tcp)
 * @brief Get TCP header length
 * @param tcp TCP structure
 * @return TCP header length value
 * @ingroup TCP
 */
INLINE uint8 tcp_get_hdr_len(const struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return tcp->header->hdr_len << TCP_HDR_LEN;
}

/**
 * @fn void tcp_set_hdr_len(struct tcp *tcp, uint8 v)
 * @brief Set TCP header length value
 * @param tcp TCP structure
 * @param v Header length value
 * @ingroup TCP
 */
INLINE void tcp_set_hdr_len(struct tcp *tcp, uint8 v)
{
	TCP_CHECK(tcp);
	tcp_pre_modify(tcp);
	tcp->header->hdr_len = v >> TCP_HDR_LEN;
}

/**
 * @fn uint8 tcp_get_flags(const struct tcp *tcp)
 * @brief Get TCP flags (fin, syn, rst, push, ack, urg, ecn, cwr)
 * @param tcp TCP structure
 * @return TCP flags
 * @ingroup TCP
 */
INLINE uint16 tcp_get_flags(const struct tcp *tcp)
{
	TCP_CHECK(tcp, 0);
	return TCP_GET_BITS(uint8, *(((uint8 *)tcp->header) + TCP_FLAGS_START), TCP_FLAGS_BITS);
}

/**
 * @fn void tcp_set_flags(struct tcp *tcp, uint8 v)
 * @brief Set TCP flags (fin, syn, rst, push, ack, urg, ecn, cwr)
 * @param tcp TCP structure
 * @param v TCP flags
 * @ingroup TCP
 */
INLINE void tcp_set_flags(struct tcp *tcp, uint8 v)
{
	TCP_CHECK(tcp);
	tcp_pre_modify(tcp);
	*(((uint8 *)tcp->header) + TCP_FLAGS_START) = TCP_SET_BITS(uint8, *(((uint8 *)tcp->header) + TCP_FLAGS_START), TCP_FLAGS_BITS, v);
}


#define TCP_GETSET_FLAG(name) \
	INLINE bool tcp_get_flags_##name(const struct tcp *tcp) { TCP_CHECK(tcp, 0); return tcp->header->name; } \
	INLINE void tcp_set_flags_##name(struct tcp *tcp, bool v) { TCP_CHECK(tcp); tcp_pre_modify(tcp); tcp->header->name = v; }

/**
 * @fn uint8 tcp_get_flags_fin(const struct tcp *tcp)
 * @brief Get TCP FIN flag
 * @param tcp TCP structure
 * @return true if FIN flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_fin(struct tcp *tcp, bool v)
 * @brief Set TCP FIN flag
 * @param tcp TCP structure
 * @param v true (FIN is set) / false (FIN is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(fin);

/**
 * @fn uint8 tcp_get_flags_syn(const struct tcp *tcp)
 * @brief Get TCP SYN flag
 * @param tcp TCP structure
 * @return true if SYN flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_syn(struct tcp *tcp, bool v)
 * @brief Set TCP SYN flag
 * @param tcp TCP structure
 * @param v true (SYN is set) / false (SYN is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(syn);

/**
 * @fn uint8 tcp_get_flags_rst(const struct tcp *tcp)
 * @brief Get TCP RST flag
 * @param tcp TCP structure
 * @return true if RST flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_rst(struct tcp *tcp, bool v)
 * @brief Set TCP RST flag
 * @param tcp TCP structure
 * @param v true (RST is set) / false (RST is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(rst);

/**
 * @fn uint8 tcp_get_flags_psh(const struct tcp *tcp)
 * @brief Get TCP PSH flag
 * @param tcp TCP structure
 * @return true if PSH flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_psh(struct tcp *tcp, bool v)
 * @brief Set TCP PSH flag
 * @param tcp TCP structure
 * @param v true (PSH is set) / false (PSH is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(psh);

/**
 * @fn uint8 tcp_get_flags_ack(const struct tcp *tcp)
 * @brief Get TCP ACK flag
 * @param tcp TCP structure
 * @return true if ACK flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_ack(struct tcp *tcp, bool v)
 * @brief Set TCP ACK flag
 * @param tcp TCP structure
 * @param v true (ACK is set) / false (ACK is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(ack);

/**
 * @fn uint8 tcp_get_flags_urg(const struct tcp *tcp)
 * @brief Get TCP URG flag
 * @param tcp TCP structure
 * @return true if URG flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_urg(struct tcp *tcp, bool v)
 * @brief Set TCP URG flag
 * @param tcp TCP structure
 * @param v true (URG is set) / false (URG is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(urg);

/**
 * @fn uint8 tcp_get_flags_ecn(const struct tcp *tcp)
 * @brief Get TCP ECN flag
 * @param tcp TCP structure
 * @return true if FIN flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_ecn(struct tcp *tcp, bool v)
 * @brief Set TCP ECN flag
 * @param tcp TCP structure
 * @param v true (ECN is set) / false (ECN is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(ecn);

/**
 * @fn uint8 tcp_get_flags_cwr(const struct tcp *tcp)
 * @brief Get TCP CWR flag
 * @param tcp TCP structure
 * @return true if CWR flag is set and false otherwise
 * @ingroup TCP
 */
/**
 * @fn void tcp_set_flags_cwr(struct tcp *tcp, bool v)
 * @brief Set TCP CWR flag
 * @param tcp TCP structure
 * @param v true (CWR is set) / false (CWR is not set)
 * @ingroup TCP
 */
TCP_GETSET_FLAG(cwr);

#endif /* _HAKA_PROTO_TCP_H */
