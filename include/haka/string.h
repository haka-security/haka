/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_STRING_H
#define _HAKA_STRING_H

#include <haka/types.h>
#include <wchar.h>

#define TOSTR(name, type, obj) \
	char name[TOSTR_##type##_size]; \
	TOSTR_##type(obj, name)

#define TOWSTR(name, type, obj) \
	wchar_t name[TOSTR_##type##_size]; \
	{ \
		char tmp[TOSTR_##type##_size]; \
		TOSTR_##type(obj, tmp); \
		swprintf(name, TOSTR_##type##_size, L"%s", tmp); \
	}

#endif /* _HAKA_STRING_H */
