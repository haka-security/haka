#include <haka/types.h>
#include <stdio.h>

#define IPV4_ADDR_STRING_MAXLEN   15
#define IPV4_NET_STRING_MAXLEN    18


/**
 * @brief Convert IP from ipv4addr to string
 * @param addr address to be converted
 * @param string converted address
 * @param size string size
 * @ingroup IPv4
 */
void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size);

/**
 * @brief Convert IP from string to ipv4addr structure
 * @param string address to be converted
 * @return ipv4addr converted address
 * @ingroup IPv4
 */
ipv4addr ipv4_addr_from_string(const char *string);

/**
 * @brief Convert IP from bytes to ipv4addr
 * @param a first IP address byte (starting from left)
 * @param b second IP address byte
 * @param c third IP address byte
 * @param d forth IP address byte
 * @return ipv4addr converted address
 * @ingroup IPv4
 */
ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d);

INLINE void ipv4_addr_to_string_sized(ipv4addr addr, char *string, size_t size, int32 *size_cp)
{
    uint8* bytes = (uint8*)&addr;
    if (size_cp) {
#if HAKA_LITTLEENDIAN
        snprintf(string, size, "%hhu.%hhu.%hhu.%hhu%n", bytes[3], bytes[2], bytes[1], bytes[0], size_cp);
#else
        snprintf(string, size, "%hhu.%hhu.%hhu.%hhu%n", bytes[0], bytes[1], bytes[2], bytes[3], size_cp);
#endif
    }
    else {
#if HAKA_LITTLEENDIAN
        snprintf(string, size, "%hhu.%hhu.%hhu.%hhu", bytes[3], bytes[2], bytes[1], bytes[0]);
#else
        snprintf(string, size, "%hhu.%hhu.%hhu.%hhu", bytes[0], bytes[1], bytes[2], bytes[3]);
#endif
    }
}

INLINE uint32 ipv4_addr_from_string_sized(const char *string, int32 *size_cp)
{
    ipv4addr addr;
    uint8* bytes = (uint8*)&addr;
    if (size_cp) {
#if HAKA_LITTLEENDIAN
        if (sscanf(string, "%hhu.%hhu.%hhu.%hhu%n", bytes+3, bytes+2, bytes+1, bytes, size_cp) != 4) {
#else
        if (sscanf(string, "%hhu.%hhu.%hhu.%hhu%n", bytes, bytes+1, bytes+2, bytes+3, size_cp) != 4) {
#endif
            return 0;
        }
    }
    else {
#if HAKA_LITTLEENDIAN
        if (sscanf(string, "%hhu.%hhu.%hhu.%hhu", bytes+3, bytes+2, bytes+1, bytes) != 4) {
#else
        if (sscanf(string, "%hhu.%hhu.%hhu.%hhu", bytes, bytes+1, bytes+2, bytes+3) != 4) {
#endif
            return 0;
        }
    }
    return addr;
}

