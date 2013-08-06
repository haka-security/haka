
#ifndef _HAKA_PROTO_IPV4_ADDR_H
#define _HAKA_PROTO_IPV4_ADDR_H

#include <haka/types.h>


#define IPV4_ADDR_STRING_MAXLEN   15

typedef uint32 ipv4addr;

#define SWAP_ipv4addr(x) SWAP_uint32(x)

void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size);
ipv4addr ipv4_addr_from_string(const char *string);
ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d);

#endif /* _HAKA_PROTO_IPV4_ADDR_H */
