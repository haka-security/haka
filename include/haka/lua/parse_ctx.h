/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HAKA_LUA_PARSE_CTX_H
#define _HAKA_LUA_PARSE_CTX_H

#define RECURS_MAX   200
#define RECURS_NODE  0
#define RECURS_LEVEL 1

struct parse_ctx {
	bool run;
	int  node;
	int  compound_level;
	/**
	 * recurs_finish_level is the current compound level when a recursion
	 * is started. The recursion will continue when we get back to this
	 * compound level.
	 */
	int  recurs_finish_level;
	int  recurs_count;
	int  recurs[RECURS_MAX][2];
};

#define PARSE_CTX_INIT { true, 1, 0, 0, 0, { { 0 } } }

#endif /* _HAKA_LUA_PARSE_CTX_H */
