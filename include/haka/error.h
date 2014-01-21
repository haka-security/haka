/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_ERROR_H
#define _HAKA_ERROR_H

#include <wchar.h>
#include <haka/types.h>


void error(const wchar_t *error, ...);
const char *errno_error(int err);
bool check_error();
const wchar_t *clear_error();

#endif /* _HAKA_ERROR_H */
