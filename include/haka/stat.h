#ifndef _HAKA_STAT_H
#define _HAKA_STAT_H

#include <haka/types.h>

bool stat_printf(FILE *out, const char *format, ...);
size_t format_bytes(size_t v, char *c);

typedef bool (*stat_callback)(FILE *out);

#endif
