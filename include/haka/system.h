/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_SYSTEM_H
#define _HAKA_SYSTEM_H

#include <haka/types.h>

bool system_register_fatal_cleanup(void (*callback)());

#endif /* _HAKA_SYSTEM_H */
