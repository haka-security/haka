#ifndef _HAKA_STAT_H
#define _HAKA_STAT_H

#include <haka/types.h>

size_t stat_printf(FILE *out, const char *format, ...);
size_t stat_format_bytes(FILE *out, size_t v);

typedef bool (*stat_callback)(FILE *out);

#endif /* _HAKA_STAT_H */
