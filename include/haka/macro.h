/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_MACRO_H
#define HAKA_MACRO_H

#define STR_(v)         #v
#define STR(v)          STR_(v)

#define CONCAT_(a, b)   a ## b
#define CONCAT(a, b)    CONCAT_(a, b)

#define MAX(a,b)        (((a)>(b))?(a):(b))
#define MIN(a,b)        (((a)<(b))?(a):(b))

#endif /* HAKA_MACRO_H */
