/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_MACRO_H
#define _HAKA_MACRO_H

#define _STR(v)		#v
#define STR(v)		_STR(v)

#define _CONCAT(a, b)   a ## b
#define CONCAT(a, b)    _CONCAT(a, b)

#endif /* _HAKA_MACRO_H */
