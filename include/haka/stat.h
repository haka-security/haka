/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_STAT_H
#define _HAKA_STAT_H

#include <haka/types.h>

size_t stat_printf(FILE *out, const char *format, ...);
size_t stat_print_formated_bytes(FILE *out, size_t v);

typedef bool (*stat_callback)(FILE *out);

#endif /* _HAKA_STAT_H */
