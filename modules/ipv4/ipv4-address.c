#include "haka/ipv4.h"
#include "haka/ipv4-address.h"

#include <haka/log.h>
#include <haka/error.h>

#include <stdlib.h>
#include <arpa/inet.h>


uint32 ipv4_addr_from_string(const char *string)
{
	ipv4addr addr;

	if (inet_pton(AF_INET, string, (struct addr_in *)&addr) <= 0) {
		error(L"Unknown IPv4 address format");
        return 0;
	}

#if HAKA_LITTLEENDIAN
	addr = SWAP_ipv4addr(addr);
#endif
	return addr;
}

void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size)
{
#if HAKA_LITTLEENDIAN
    addr = SWAP_ipv4addr(addr);
#endif
	inet_ntop(AF_INET, &addr, string, size);
}


ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d)
{
    ipv4addr addr;
    uint8* bytes = (uint8*)&addr;

#if HAKA_LITTLEENDIAN
    bytes[3] = a; bytes[2] = b; bytes[1] = c; bytes[0] = d;
#else
    bytes[0] = a; bytes[1] = b; bytes[2] = c; bytes[3] = d;
#endif
    return addr;
}

